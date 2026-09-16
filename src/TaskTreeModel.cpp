#include "polomodoro/TaskTreeModel.h"

#include "polomodoro/TaskTree.h"

#include <QDateTime>

namespace polomodoro {

TaskTreeModel::TaskTreeModel(TaskTree &tree, QObject *parent)
    : QAbstractItemModel(parent), m_tree(tree)
{
}

void TaskTreeModel::setBucketFilter(const QString &bucket)
{
    beginResetModel();
    m_bucketFilter = bucket.toLower();
    m_activeSubtree = false;
    rebuild();
    endResetModel();
}

void TaskTreeModel::setActiveSubtreeFilter(bool enabled)
{
    beginResetModel();
    m_activeSubtree = enabled;
    rebuild();
    endResetModel();
}

void TaskTreeModel::setProgressBasis(const QString &basis)
{
    m_progressBasis = basis;
    emit dataChanged(index(0, 0), index(rowCount() - 1, 0));
}

void TaskTreeModel::setShowCompleted(bool enabled)
{
    if (m_showCompleted == enabled)
        return;
    beginResetModel();
    m_showCompleted = enabled;
    rebuild();
    endResetModel();
}

void TaskTreeModel::refresh()
{
    beginResetModel();
    rebuild();
    endResetModel();
}

// Called every second by TaskController's live timer. Must never do a full
// reset: that tears down and recreates every delegate, which briefly reads
// undefined role values while QML re-binds (visible as blank/flickering
// rows). Bucket membership never changes here, only the time-derived roles.
void TaskTreeModel::tick()
{
    if (m_rows.isEmpty())
        return;
    static const QList<int> liveRoles = {
        LiveElapsedMsRole, TotalElapsedMsRole, ProgressRatioRole, RemainingMsRole,
        ProgressLabelRole, OverflowRatioRole, OverTargetRole, BadgeTextRole,
        StartableRole, OverdueRole, TargetReachedRole, HasTargetRole,
    };
    emit dataChanged(index(0, 0), index(m_rows.size() - 1, 0), liveRoles);
}

void TaskTreeModel::rebuild()
{
    m_rows.clear();
    TaskListBucket bucket = TaskListBucket::All;
    if (m_bucketFilter == QStringLiteral("future"))
        bucket = TaskListBucket::Future;
    else if (m_bucketFilter == QStringLiteral("pending"))
        bucket = TaskListBucket::Pending;
    else if (m_bucketFilter == QStringLiteral("active"))
        bucket = TaskListBucket::Active;

    if (m_activeSubtree) {
        for (const TaskNode *node : m_tree.activeSubtree()) {
            FlatRow row;
            row.tree = &m_tree;
            row.nodePtr = node;
            row.parentId = node->parentId;
            row.depth = 0;
            row.hasPrevSibling = false;
            m_rows.push_back(row);
        }
        return;
    }

    for (const auto &flat : m_tree.flattenBucket(bucket, m_showCompleted)) {
        FlatRow row;
        row.tree = &m_tree;
        row.nodePtr = flat.node;
        row.parentId = flat.node->parentId;
        row.depth = flat.depth;
        row.hasPrevSibling = flat.hasPrevSibling;
        m_rows.push_back(row);
    }
}

bool TaskTreeModel::rowMatchesBucket(const TaskNode &node) const
{
    Q_UNUSED(node);
    return true;
}

QModelIndex TaskTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= m_rows.size() || column != 0)
        return {};
    return createIndex(row, column, row);
}

QModelIndex TaskTreeModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return {};
}

int TaskTreeModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

int TaskTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant TaskTreeModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_rows.size())
        return {};

    const TaskNode *node = static_cast<const TaskNode *>(m_rows.at(idx.row()).nodePtr);
    if (!node)
        return {};

    switch (role) {
    case IdRole: return node->id;
    case TitleRole: return node->title;
    case StatusRole: {
        switch (node->status) {
        case TaskStatus::Idle: return QStringLiteral("idle");
        case TaskStatus::Active: return QStringLiteral("active");
        case TaskStatus::Paused: return QStringLiteral("paused");
        case TaskStatus::Stopped: return QStringLiteral("stopped");
        case TaskStatus::Completed: return QStringLiteral("completed");
        }
        return QStringLiteral("idle");
    }
    case IsActiveRole: return node->status == TaskStatus::Active;
    case SelfElapsedMsRole: return node->selfElapsedMs;
    case LiveElapsedMsRole: return m_tree.liveElapsedMs(*node);
    case TotalElapsedMsRole: return m_tree.liveElapsedMs(*node);
    case ActiveSinceRole: return node->activeSince;
    case ScheduledStartAtRole: return node->scheduledStartAt;
    case ScheduledEndAtRole: return node->scheduledEndAt;
    case TargetMsRole: return node->targetMs;
    case ProgressRatioRole: return m_tree.progressRatio(*node, m_progressBasis);
    case RemainingMsRole: {
        if (node->targetMs <= 0) return 0;
        return qMax<qint64>(0, node->targetMs - m_tree.progressMs(*node, m_progressBasis));
    }
    case HasTargetRole: return node->targetMs > 0;
    case TargetReachedRole: return node->targetReachedAt.isValid();
    case DepthRole: return m_rows.at(idx.row()).depth;
    case ProgressLabelRole: {
        const qint64 ms = m_tree.progressMs(*node, m_progressBasis);
        const qint64 sec = ms / 1000;
        const qint64 h = sec / 3600;
        const qint64 m = (sec % 3600) / 60;
        const qint64 s = sec % 60;
        if (h > 0)
            return QStringLiteral("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        return QStringLiteral("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
    }
    case OverflowRatioRole: {
        if (node->targetMs <= 0)
            return 0.0;
        const double ratio = m_tree.progressRatio(*node, m_progressBasis);
        return qMax(0.0, ratio - 1.0);
    }
    case OverTargetRole:
        return node->targetMs > 0 && m_tree.progressRatio(*node, m_progressBasis) > 1.0;
    case BadgeTextRole: {
        if (node->targetMs > 0) {
            const qint64 remaining = qMax<qint64>(0, node->targetMs - m_tree.progressMs(*node, m_progressBasis));
            const qint64 sec = remaining / 1000;
            return QStringLiteral("%1m left").arg((sec + 59) / 60);
        }
        return QString();
    }
    case StartableRole:
        return node->status == TaskStatus::Idle || node->status == TaskStatus::Stopped;
    case OverdueRole:
        return node->scheduledEndAt.isValid() && node->scheduledEndAt < QDateTime::currentDateTimeUtc()
               && node->status != TaskStatus::Completed;
    case StartsAtLabelRole:
        return node->scheduledStartAt.isValid() ? node->scheduledStartAt.toLocalTime().toString(Qt::ISODate) : QString();
    case HasChildrenRole:
        return !node->children.isEmpty();
    case HasPrevSiblingRole:
        return m_rows.at(idx.row()).hasPrevSibling;
    case Qt::DisplayRole: return node->title;
    default: return {};
    }
}

QHash<int, QByteArray> TaskTreeModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {TitleRole, "title"},
        {StatusRole, "status"},
        {IsActiveRole, "isActive"},
        {SelfElapsedMsRole, "selfElapsedMs"},
        {LiveElapsedMsRole, "liveElapsedMs"},
        {TotalElapsedMsRole, "totalElapsedMs"},
        {ActiveSinceRole, "activeSince"},
        {ScheduledStartAtRole, "scheduledStartAt"},
        {ScheduledEndAtRole, "scheduledEndAt"},
        {TargetMsRole, "targetMs"},
        {ProgressRatioRole, "progressRatio"},
        {RemainingMsRole, "remainingMs"},
        {HasTargetRole, "hasTarget"},
        {TargetReachedRole, "targetReached"},
        {DepthRole, "depth"},
        {ProgressLabelRole, "progressLabel"},
        {OverflowRatioRole, "overflowRatio"},
        {OverTargetRole, "overTarget"},
        {BadgeTextRole, "badgeText"},
        {StartableRole, "startable"},
        {OverdueRole, "overdue"},
        {StartsAtLabelRole, "startsAtLabel"},
        {HasChildrenRole, "hasChildren"},
        {HasPrevSiblingRole, "hasPrevSibling"},
    };
}

} // namespace polomodoro
