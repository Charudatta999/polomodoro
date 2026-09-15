#include "polomodoro/TaskController.h"

#include "polomodoro/SettingsStore.h"
#include "polomodoro/TaskTree.h"
#include "polomodoro/TaskTreeModel.h"

#include <QTimer>

namespace polomodoro {

struct TaskController::Impl {
    TaskTree *tree = nullptr;
    SettingsStore *settings = nullptr;
    std::unique_ptr<TaskTreeModel> model;
    std::unique_ptr<TaskTreeModel> activeModel;
    std::unique_ptr<TaskTreeModel> pendingModel;
    std::unique_ptr<TaskTreeModel> futureModel;
    std::unique_ptr<TaskTreeModel> allModel;
    std::unique_ptr<TaskTreeModel> activeSubtreeModel;
    QTimer liveTimer;
    bool menuOpen = false;
    bool showCompleted = false;
    QString progressBasis;
};

TaskController::TaskController(TaskTree &tree, SettingsStore &settings, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>())
{
    d->tree = &tree;
    d->settings = &settings;
    d->progressBasis = d->settings->getString(QStringLiteral("targetProgressBasis"), QStringLiteral("active"));
    d->model = std::make_unique<TaskTreeModel>(*d->tree, this);
    d->activeModel = std::make_unique<TaskTreeModel>(*d->tree, this);
    d->pendingModel = std::make_unique<TaskTreeModel>(*d->tree, this);
    d->futureModel = std::make_unique<TaskTreeModel>(*d->tree, this);
    d->allModel = std::make_unique<TaskTreeModel>(*d->tree, this);
    d->activeSubtreeModel = std::make_unique<TaskTreeModel>(*d->tree, this);
    d->activeModel->setBucketFilter(QStringLiteral("active"));
    d->pendingModel->setBucketFilter(QStringLiteral("pending"));
    d->futureModel->setBucketFilter(QStringLiteral("future"));
    d->allModel->setBucketFilter(QStringLiteral("all"));
    d->activeSubtreeModel->setActiveSubtreeFilter(true);
    d->model->setProgressBasis(d->progressBasis);
    for (TaskTreeModel *m : {d->activeModel.get(), d->pendingModel.get(), d->futureModel.get(), d->allModel.get(), d->activeSubtreeModel.get()})
        m->setProgressBasis(d->progressBasis);

    d->liveTimer.setInterval(1000);
    connect(&d->liveTimer, &QTimer::timeout, this, [this]() {
        d->tree->promoteFutureTasks();
        refreshAllModels();
        emit tasksChanged();
    });
    d->liveTimer.start();
}

TaskController::~TaskController() = default;

TaskTreeModel *TaskController::model() const { return d->model.get(); }
TaskTreeModel *TaskController::activeSubtreeProxy() const { return d->activeSubtreeModel.get(); }
double TaskController::overallProgressRatio() const { return overallRatio(); }
double TaskController::overallRatio() const { return d->tree->overallProgressRatio(d->progressBasis); }
qint64 TaskController::overallTargetMs() const
{
    qint64 total = 0;
    for (const TaskNode *task : d->tree->activeTasks()) {
        if (task->targetMs > 0)
            total += task->targetMs;
    }
    return total;
}
int TaskController::activeTaskCount() const { return activeCount(); }
int TaskController::activeCount() const { return int(d->tree->activeTasks().size()); }
int TaskController::pendingCount() const { return d->tree->tasksInBucket(TaskListBucket::Pending).size(); }
int TaskController::futureCount() const { return d->tree->tasksInBucket(TaskListBucket::Future).size(); }

int TaskController::activeOverflowCount() const
{
    const int count = activeCount();
    return count > 4 ? count - 4 : 0;
}

QVariantList TaskController::activeChips() const
{
    QVariantList chips;
    const auto tasks = d->tree->activeTasks();
    const int limit = qMin(4, tasks.size());
    for (int i = 0; i < limit; ++i) {
        const TaskNode *task = tasks.at(i);
        const qint64 ms = d->tree->progressMs(*task, d->progressBasis);
        const bool over = task->targetMs > 0 && ms > task->targetMs;
        chips.push_back(QVariantMap{
            {QStringLiteral("id"), task->id},
            {QStringLiteral("title"), task->title},
            {QStringLiteral("liveLabel"), formatDuration(ms)},
            {QStringLiteral("overTarget"), over},
        });
    }
    return chips;
}

QString TaskController::combinedActiveLabel() const
{
    qint64 total = 0;
    for (const TaskNode *task : d->tree->activeTasks())
        total += d->tree->progressMs(*task, d->progressBasis);
    return formatDuration(total);
}

QVariant TaskController::soleTargetedActiveTask() const
{
    const auto tasks = d->tree->activeTasks();
    int targeted = 0;
    const TaskNode *sole = nullptr;
    for (const TaskNode *task : tasks) {
        if (task->targetMs > 0) {
            ++targeted;
            sole = task;
        }
    }
    if (targeted != 1 || !sole)
        return {};
    return QVariantMap{
        {QStringLiteral("title"), sole->title},
        {QStringLiteral("progressLabel"), formatDuration(d->tree->progressMs(*sole, d->progressBasis))},
        {QStringLiteral("progressRatio"), d->tree->progressRatio(*sole, d->progressBasis)},
    };
}

bool TaskController::menuOpen() const { return d->menuOpen; }
bool TaskController::showCompleted() const { return d->showCompleted; }

TaskTreeModel *TaskController::proxyFor(const QString &bucket)
{
    const QString key = bucket.toLower();
    if (key == QStringLiteral("active"))
        return d->activeModel.get();
    if (key == QStringLiteral("pending"))
        return d->pendingModel.get();
    if (key == QStringLiteral("future"))
        return d->futureModel.get();
    return d->allModel.get();
}

void TaskController::setMenuOpen(bool open)
{
    if (d->menuOpen == open)
        return;
    d->menuOpen = open;
    emit menuOpenChanged();
}

void TaskController::setBucketFilter(const QString &bucket) { d->model->setBucketFilter(bucket); }

void TaskController::setProgressBasis(const QString &basis)
{
    d->progressBasis = basis;
    d->settings->setString(QStringLiteral("targetProgressBasis"), basis);
    for (TaskTreeModel *m : {d->model.get(), d->activeModel.get(), d->pendingModel.get(), d->futureModel.get(), d->allModel.get(), d->activeSubtreeModel.get()})
        m->setProgressBasis(basis);
}

void TaskController::setShowCompleted(bool value)
{
    if (d->showCompleted == value)
        return;
    d->showCompleted = value;
    refreshAllModels();
    emit tasksChanged();
}

void TaskController::refreshAllModels()
{
    d->model->refresh();
    d->activeModel->refresh();
    d->pendingModel->refresh();
    d->futureModel->refresh();
    d->allModel->refresh();
    d->activeSubtreeModel->refresh();
}

QString TaskController::createTask(const QString &title, const QString &parentId)
{
    const QString id = d->tree->createTask(title, parentId);
    refreshAllModels();
    emit tasksChanged();
    return id;
}

void TaskController::startTask(const QString &id)
{
    d->tree->startTask(id);
    refreshAllModels();
    emit tasksChanged();
}

void TaskController::pauseTask(const QString &id)
{
    d->tree->pauseTask(id);
    refreshAllModels();
    emit tasksChanged();
}

void TaskController::resumeTask(const QString &id)
{
    d->tree->resumeTask(id);
    refreshAllModels();
    emit tasksChanged();
}

void TaskController::stopTask(const QString &id)
{
    d->tree->stopTask(id);
    refreshAllModels();
    emit tasksChanged();
}

void TaskController::completeTask(const QString &id)
{
    d->tree->completeTask(id);
    refreshAllModels();
    emit tasksChanged();
}

void TaskController::deleteTask(const QString &id)
{
    d->tree->deleteTask(id);
    refreshAllModels();
    emit tasksChanged();
}

void TaskController::promote(const QString &id) { Q_UNUSED(id); }
void TaskController::demote(const QString &id) { Q_UNUSED(id); }
void TaskController::requestEdit(const QString &id) { emit editRequested(id); }
void TaskController::requestCreate(const QString &parentId) { emit createRequested(parentId); }
void TaskController::loadInto(QObject *editor, const QString &id)
{
    if (!editor)
        return;
    const TaskNode *node = d->tree->findById(id);
    if (!node)
        return;
    editor->setProperty("taskId", id);
    editor->setProperty("title", node->title);
}
void TaskController::openDrawerOnActive()
{
    setMenuOpen(true);
    setBucketFilter(QStringLiteral("active"));
}

void TaskController::updateTaskTitle(const QString &id, const QString &title)
{
    if (TaskNode *node = d->tree->findById(id)) {
        node->title = title;
        d->tree->saveTask(id);
        refreshAllModels();
        emit tasksChanged();
    }
}

void TaskController::updateTaskTargetMs(const QString &id, qint64 targetMs)
{
    if (TaskNode *node = d->tree->findById(id)) {
        node->targetMs = targetMs;
        d->tree->saveTask(id);
        refreshAllModels();
        emit tasksChanged();
    }
}

void TaskController::updateTaskSchedule(const QString &id, const QString &startIso, const QString &endIso)
{
    if (TaskNode *node = d->tree->findById(id)) {
        node->scheduledStartAt = startIso.isEmpty() ? QDateTime() : QDateTime::fromString(startIso, Qt::ISODateWithMs);
        node->scheduledEndAt = endIso.isEmpty() ? QDateTime() : QDateTime::fromString(endIso, Qt::ISODateWithMs);
        d->tree->saveTask(id);
        refreshAllModels();
        emit tasksChanged();
    }
}

QString TaskController::formatDuration(qint64 ms) const
{
    const qint64 sec = ms / 1000;
    const qint64 h = sec / 3600;
    const qint64 m = (sec % 3600) / 60;
    if (h > 0)
        return QStringLiteral("%1h %2m").arg(h).arg(m);
    return QStringLiteral("%1m").arg(m);
}

void TaskController::refresh()
{
    refreshAllModels();
    emit tasksChanged();
}

} // namespace polomodoro
