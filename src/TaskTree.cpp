#include "polomodoro/TaskTree.h"

#include "polomodoro/DatabaseManager.h"

#include <algorithm>
#include <functional>

#include <QHash>
#include <QSqlQuery>
#include <QUuid>

namespace polomodoro {

namespace {

QString statusToString(TaskStatus status)
{
    switch (status) {
    case TaskStatus::Idle: return QStringLiteral("idle");
    case TaskStatus::Active: return QStringLiteral("active");
    case TaskStatus::Paused: return QStringLiteral("paused");
    case TaskStatus::Stopped: return QStringLiteral("stopped");
    case TaskStatus::Completed: return QStringLiteral("completed");
    }
    return QStringLiteral("idle");
}

TaskStatus statusFromString(const QString &s)
{
    if (s == QLatin1String("active")) return TaskStatus::Active;
    if (s == QLatin1String("paused")) return TaskStatus::Paused;
    if (s == QLatin1String("stopped")) return TaskStatus::Stopped;
    if (s == QLatin1String("completed")) return TaskStatus::Completed;
    return TaskStatus::Idle;
}

TaskNode nodeFromQuery(const QSqlQuery &q)
{
    TaskNode n;
    n.id = q.value(QStringLiteral("id")).toString();
    n.title = q.value(QStringLiteral("title")).toString();
    n.parentId = q.value(QStringLiteral("parent_id")).toString();
    n.sortOrder = q.value(QStringLiteral("sort_order")).toInt();
    n.status = statusFromString(q.value(QStringLiteral("status")).toString());
    const auto activeSince = q.value(QStringLiteral("active_since")).toString();
    if (!activeSince.isEmpty())
        n.activeSince = QDateTime::fromString(activeSince, Qt::ISODateWithMs);
    const auto schedStart = q.value(QStringLiteral("scheduled_start_at")).toString();
    if (!schedStart.isEmpty())
        n.scheduledStartAt = QDateTime::fromString(schedStart, Qt::ISODateWithMs);
    const auto schedEnd = q.value(QStringLiteral("scheduled_end_at")).toString();
    if (!schedEnd.isEmpty())
        n.scheduledEndAt = QDateTime::fromString(schedEnd, Qt::ISODateWithMs);
    n.selfElapsedMs = q.value(QStringLiteral("self_elapsed_ms")).toLongLong();
    if (!q.value(QStringLiteral("target_ms")).isNull())
        n.targetMs = q.value(QStringLiteral("target_ms")).toLongLong();
    const auto targetReached = q.value(QStringLiteral("target_reached_at")).toString();
    if (!targetReached.isEmpty())
        n.targetReachedAt = QDateTime::fromString(targetReached, Qt::ISODateWithMs);
    n.createdAt = QDateTime::fromString(q.value(QStringLiteral("created_at")).toString(), Qt::ISODateWithMs);
    n.updatedAt = QDateTime::fromString(q.value(QStringLiteral("updated_at")).toString(), Qt::ISODateWithMs);
    return n;
}

void appendChildren(TaskNode &parent, QHash<QString, QVector<TaskNode>> &byParent)
{
    auto children = byParent.take(parent.id);
    std::sort(children.begin(), children.end(), [](const TaskNode &a, const TaskNode &b) {
        return a.sortOrder < b.sortOrder;
    });
    for (auto &child : children) {
        appendChildren(child, byParent);
        parent.children.push_back(std::move(child));
    }
}

TaskNode *findMutable(TaskNode &node, const QString &id)
{
    if (node.id == id)
        return &node;
    for (auto &child : node.children) {
        if (auto *found = findMutable(child, id))
            return found;
    }
    return nullptr;
}

const TaskNode *findConst(const TaskNode &node, const QString &id)
{
    if (node.id == id)
        return &node;
    for (const auto &child : node.children) {
        if (const auto *found = findConst(child, id))
            return found;
    }
    return nullptr;
}

void walk(const TaskNode &node, const std::function<void(const TaskNode &)> &fn)
{
    fn(node);
    for (const auto &child : node.children)
        walk(child, fn);
}

void walkMutable(TaskNode &node, const std::function<void(TaskNode &)> &fn)
{
    fn(node);
    for (auto &child : node.children)
        walkMutable(child, fn);
}

qint64 totalElapsedRecursive(const TaskTree &tree, const TaskNode &node)
{
    qint64 total = tree.liveElapsedMs(node);
    for (const auto &child : node.children)
        total += totalElapsedRecursive(tree, child);
    return total;
}

} // namespace

struct TaskTree::Impl {
    DatabaseManager &db;
    QVector<TaskNode> roots;
};

TaskTree::TaskTree(DatabaseManager &db) : d(std::make_unique<Impl>(Impl{db})) {}
TaskTree::~TaskTree() = default;

const QVector<TaskNode> &TaskTree::roots() const { return d->roots; }

bool TaskTree::load()
{
    loadFlat();
    reconcileOnStartup();
    promoteFutureTasks();
    return true;
}

void TaskTree::loadFlat()
{
    d->roots.clear();
    QHash<QString, QVector<TaskNode>> byParent;
    QSqlQuery q(d->db.database());
    q.exec(QStringLiteral("SELECT * FROM tasks ORDER BY sort_order"));
    while (q.next()) {
        TaskNode n = nodeFromQuery(q);
        byParent[n.parentId].push_back(std::move(n));
    }
    auto roots = byParent.take(QString());
    std::sort(roots.begin(), roots.end(), [](const TaskNode &a, const TaskNode &b) {
        return a.sortOrder < b.sortOrder;
    });
    for (auto &root : roots) {
        appendChildren(root, byParent);
        d->roots.push_back(std::move(root));
    }
}

bool TaskTree::saveTask(const QString &id)
{
    TaskNode *node = findById(id);
    if (!node)
        return false;

    QSqlQuery q(d->db.database());
    q.prepare(QStringLiteral(
        "INSERT INTO tasks(id, title, parent_id, sort_order, status, active_since, "
        "scheduled_start_at, scheduled_end_at, self_elapsed_ms, target_ms, target_reached_at, created_at, updated_at) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(id) DO UPDATE SET title=excluded.title, parent_id=excluded.parent_id, sort_order=excluded.sort_order, "
        "status=excluded.status, active_since=excluded.active_since, scheduled_start_at=excluded.scheduled_start_at, "
        "scheduled_end_at=excluded.scheduled_end_at, self_elapsed_ms=excluded.self_elapsed_ms, target_ms=excluded.target_ms, "
        "target_reached_at=excluded.target_reached_at, updated_at=excluded.updated_at"));
    q.addBindValue(node->id);
    q.addBindValue(node->title);
    q.addBindValue(node->parentId.isEmpty() ? QVariant() : node->parentId);
    q.addBindValue(node->sortOrder);
    q.addBindValue(statusToString(node->status));
    q.addBindValue(node->activeSince.isValid() ? node->activeSince.toUTC().toString(Qt::ISODateWithMs) : QVariant());
    q.addBindValue(node->scheduledStartAt.isValid() ? node->scheduledStartAt.toUTC().toString(Qt::ISODateWithMs) : QVariant());
    q.addBindValue(node->scheduledEndAt.isValid() ? node->scheduledEndAt.toUTC().toString(Qt::ISODateWithMs) : QVariant());
    q.addBindValue(node->selfElapsedMs);
    q.addBindValue(node->targetMs >= 0 ? node->targetMs : QVariant());
    q.addBindValue(node->targetReachedAt.isValid() ? node->targetReachedAt.toUTC().toString(Qt::ISODateWithMs) : QVariant());
    q.addBindValue(node->createdAt.toUTC().toString(Qt::ISODateWithMs));
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    return q.exec();
}

bool TaskTree::saveAll()
{
    bool ok = true;
    for (const auto &root : d->roots) {
        walk(root, [&](const TaskNode &n) {
            ok = saveTask(n.id) && ok;
        });
    }
    return ok;
}

void TaskTree::heartbeat()
{
    d->db.setAppState(QStringLiteral("last_saved_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    d->db.setAppState(QStringLiteral("app_version"), QStringLiteral("1.0.0"));
}

QString TaskTree::createTask(const QString &title, const QString &parentId)
{
    TaskNode n;
    n.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    n.title = title;
    n.parentId = parentId;
    n.createdAt = QDateTime::currentDateTimeUtc();
    n.updatedAt = n.createdAt;
    saveTask(n.id);

    if (parentId.isEmpty()) {
        d->roots.push_back(n);
    } else if (TaskNode *parent = findById(parentId)) {
        parent->children.push_back(n);
    }
    QSqlQuery q(d->db.database());
    q.prepare(QStringLiteral(
        "INSERT INTO tasks(id, title, parent_id, sort_order, status, self_elapsed_ms, created_at, updated_at) "
        "VALUES(?,?,?,?,?,?,?,?)"));
    q.addBindValue(n.id);
    q.addBindValue(n.title);
    q.addBindValue(parentId.isEmpty() ? QVariant() : parentId);
    q.addBindValue(n.sortOrder);
    q.addBindValue(statusToString(n.status));
    q.addBindValue(n.selfElapsedMs);
    q.addBindValue(n.createdAt.toString(Qt::ISODateWithMs));
    q.addBindValue(n.updatedAt.toString(Qt::ISODateWithMs));
    q.exec();
    return n.id;
}

bool TaskTree::updateTask(const TaskNode &node)
{
    if (!findById(node.id))
        return false;
    *findById(node.id) = node;
    return saveTask(node.id);
}

bool TaskTree::deleteTask(const QString &id)
{
    QSqlQuery q(d->db.database());
    q.prepare(QStringLiteral("DELETE FROM tasks WHERE id=?"));
    q.addBindValue(id);
    if (!q.exec())
        return false;
    loadFlat();
    return true;
}

namespace {

bool removeFromVector(QVector<TaskNode> &vec, const QString &id, TaskNode &out)
{
    for (int i = 0; i < vec.size(); ++i) {
        if (vec[i].id == id) {
            out = vec[i];
            vec.remove(i);
            return true;
        }
    }
    for (auto &n : vec) {
        if (removeFromVector(n.children, id, out))
            return true;
    }
    return false;
}

} // namespace

bool TaskTree::reparentTask(const QString &id, const QString &newParentId)
{
    if (id == newParentId)
        return false;
    TaskNode *node = findById(id);
    if (!node)
        return false;
    if (node->parentId == newParentId)
        return true;
    if (!newParentId.isEmpty()) {
        if (!findById(newParentId))
            return false;
        bool isDescendant = false;
        walk(*node, [&](const TaskNode &n) {
            if (n.id == newParentId)
                isDescendant = true;
        });
        if (isDescendant)
            return false;
    }

    TaskNode moved;
    if (!removeFromVector(d->roots, id, moved))
        return false;
    moved.parentId = newParentId;

    if (!newParentId.isEmpty()) {
        if (TaskNode *parent = findById(newParentId)) {
            parent->children.push_back(std::move(moved));
        } else {
            d->roots.push_back(std::move(moved));
        }
    } else {
        d->roots.push_back(std::move(moved));
    }
    return saveTask(id);
}

void TaskTree::finalizeActiveSegment(TaskNode &node, const QDateTime &now)
{
    if (node.status == TaskStatus::Active && node.activeSince.isValid()) {
        node.selfElapsedMs += now.toMSecsSinceEpoch() - node.activeSince.toMSecsSinceEpoch();
        node.activeSince = {};
    }
}

bool TaskTree::startTask(const QString &id)
{
    TaskNode *node = findById(id);
    if (!node || node->status == TaskStatus::Completed)
        return false;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    node->status = TaskStatus::Active;
    node->activeSince = now;
    return saveTask(id);
}

bool TaskTree::pauseTask(const QString &id)
{
    TaskNode *node = findById(id);
    if (!node || node->status != TaskStatus::Active)
        return false;
    finalizeActiveSegment(*node, QDateTime::currentDateTimeUtc());
    node->status = TaskStatus::Paused;
    return saveTask(id);
}

bool TaskTree::resumeTask(const QString &id)
{
    TaskNode *node = findById(id);
    if (!node || node->status != TaskStatus::Paused)
        return false;
    node->status = TaskStatus::Active;
    node->activeSince = QDateTime::currentDateTimeUtc();
    return saveTask(id);
}

bool TaskTree::stopTask(const QString &id)
{
    TaskNode *node = findById(id);
    if (!node)
        return false;
    if (node->status == TaskStatus::Active)
        finalizeActiveSegment(*node, QDateTime::currentDateTimeUtc());
    node->status = TaskStatus::Stopped;
    node->activeSince = {};
    return saveTask(id);
}

bool TaskTree::completeTask(const QString &id)
{
    TaskNode *node = findById(id);
    if (!node)
        return false;
    if (node->status == TaskStatus::Active)
        finalizeActiveSegment(*node, QDateTime::currentDateTimeUtc());
    node->status = TaskStatus::Completed;
    node->activeSince = {};
    return saveTask(id);
}

TaskNode *TaskTree::findById(const QString &id)
{
    for (auto &root : d->roots) {
        if (auto *found = findMutable(root, id))
            return found;
    }
    return nullptr;
}

const TaskNode *TaskTree::findById(const QString &id) const
{
    for (const auto &root : d->roots) {
        if (const auto *found = findConst(root, id))
            return found;
    }
    return nullptr;
}

qint64 TaskTree::liveElapsedMs(const TaskNode &node) const
{
    qint64 ms = node.selfElapsedMs;
    if (node.status == TaskStatus::Active && node.activeSince.isValid())
        ms += QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - node.activeSince.toMSecsSinceEpoch();
    return ms;
}

qint64 TaskTree::loggedWorkMs(const QString &taskId) const
{
    QSqlQuery q(d->db.database());
    q.prepare(QStringLiteral("SELECT COALESCE(SUM(duration_ms),0) FROM sessions WHERE task_id=?"));
    q.addBindValue(taskId);
    if (q.exec() && q.next())
        return q.value(0).toLongLong();
    return 0;
}

qint64 TaskTree::progressMs(const TaskNode &node, const QString &basis) const
{
    return basis == QStringLiteral("logged") ? loggedWorkMs(node.id) : liveElapsedMs(node);
}

double TaskTree::progressRatio(const TaskNode &node, const QString &basis) const
{
    if (node.targetMs <= 0)
        return 0.0;
    return qMin(1.0, static_cast<double>(progressMs(node, basis)) / static_cast<double>(node.targetMs));
}

QVector<const TaskNode *> TaskTree::activeTasks() const
{
    QVector<const TaskNode *> result;
    for (const auto &root : d->roots) {
        walk(root, [&](const TaskNode &n) {
            if (n.status == TaskStatus::Active)
                result.push_back(findById(n.id));
        });
    }
    return result;
}

static bool isFuture(const TaskNode &n)
{
    return n.scheduledStartAt.isValid() && n.scheduledStartAt > QDateTime::currentDateTimeUtc()
           && n.status != TaskStatus::Completed;
}

static bool isPending(const TaskNode &n)
{
    if (n.status == TaskStatus::Completed || n.status == TaskStatus::Active)
        return false;
    if (isFuture(n))
        return false;
    return n.status == TaskStatus::Idle || n.status == TaskStatus::Paused || n.status == TaskStatus::Stopped;
}

QVector<const TaskNode *> TaskTree::tasksInBucket(TaskListBucket bucket) const
{
    QVector<const TaskNode *> result;
    for (const auto &root : d->roots) {
        walk(root, [&](const TaskNode &n) {
            bool include = false;
            switch (bucket) {
            case TaskListBucket::Future: include = isFuture(n); break;
            case TaskListBucket::Pending: include = isPending(n); break;
            case TaskListBucket::Active: include = n.status == TaskStatus::Active; break;
            case TaskListBucket::All: include = n.status != TaskStatus::Completed; break;
            }
            if (include)
                result.push_back(findById(n.id));
        });
    }
    return result;
}

QVector<const TaskNode *> TaskTree::activeSubtree() const
{
    QVector<const TaskNode *> result;
    for (const auto *active : activeTasks()) {
        walk(*active, [&](const TaskNode &n) {
            result.push_back(findById(n.id));
        });
    }
    return result;
}

namespace {

bool matchesBucketFor(const TaskNode &n, TaskListBucket bucket, bool showCompleted)
{
    switch (bucket) {
    case TaskListBucket::Future: return isFuture(n);
    case TaskListBucket::Pending: return isPending(n);
    case TaskListBucket::Active: return n.status == TaskStatus::Active;
    case TaskListBucket::All: return showCompleted || n.status != TaskStatus::Completed;
    }
    return false;
}

bool subtreeHasMatch(const TaskNode &n, TaskListBucket bucket, bool showCompleted)
{
    if (matchesBucketFor(n, bucket, showCompleted))
        return true;
    for (const auto &child : n.children) {
        if (subtreeHasMatch(child, bucket, showCompleted))
            return true;
    }
    return false;
}

} // namespace

QVector<TaskTree::FlatTreeRow> TaskTree::flattenBucket(TaskListBucket bucket, bool showCompleted) const
{
    QVector<FlatTreeRow> result;

    std::function<bool(const TaskNode &, int)> visit = [&](const TaskNode &n, int depth) -> bool {
        if (!subtreeHasMatch(n, bucket, showCompleted))
            return false;
        FlatTreeRow row;
        row.node = findById(n.id);
        row.depth = depth;
        row.hasPrevSibling = false;
        result.push_back(row);
        bool anyChildVisible = false;
        for (const auto &child : n.children) {
            const int childRowIndex = result.size();
            const bool visible = visit(child, depth + 1);
            if (visible) {
                result[childRowIndex].hasPrevSibling = anyChildVisible;
                anyChildVisible = true;
            }
        }
        return true;
    };

    for (const auto &root : d->roots)
        visit(root, 0);

    return result;
}

double TaskTree::overallProgressRatio(const QString &basis) const
{
    qint64 totalProgress = 0;
    qint64 totalTarget = 0;
    for (const auto *task : activeTasks()) {
        if (task->targetMs > 0) {
            totalProgress += progressMs(*task, basis);
            totalTarget += task->targetMs;
        }
    }
    if (totalTarget <= 0)
        return 0.0;
    return qMin(1.0, static_cast<double>(totalProgress) / static_cast<double>(totalTarget));
}

void TaskTree::addSession(const QString &taskId, const QDateTime &startedAt, qint64 durationMs, const QString &mode)
{
    QSqlQuery q(d->db.database());
    q.prepare(QStringLiteral("INSERT INTO sessions(task_id, started_at, duration_ms, mode) VALUES(?,?,?,?)"));
    q.addBindValue(taskId);
    q.addBindValue(startedAt.toUTC().toString(Qt::ISODateWithMs));
    q.addBindValue(durationMs);
    q.addBindValue(mode);
    q.exec();
}

void TaskTree::reconcileOnStartup()
{
    const QString lastSaved = d->db.appState(QStringLiteral("last_saved_at"));
    for (auto &root : d->roots) {
        walkMutable(root, [&](TaskNode &n) {
            if (n.status == TaskStatus::Active && !n.activeSince.isValid() && !lastSaved.isEmpty())
                n.activeSince = QDateTime::fromString(lastSaved, Qt::ISODateWithMs);
        });
    }
}

void TaskTree::promoteFutureTasks()
{
    // Future tasks become pending automatically when start time passes (no status change needed)
}

bool TaskTree::anyTaskActive() const
{
    return !activeTasks().isEmpty();
}

void TaskTree::recomputeTotals(TaskNode &node) { Q_UNUSED(node); }

} // namespace polomodoro
