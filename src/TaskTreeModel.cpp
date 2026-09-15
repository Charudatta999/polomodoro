#include "polomodoro/TaskTreeModel.h"

#include "polomodoro/TaskTree.h"

namespace polomodoro {

TaskTreeModel::TaskTreeModel(TaskTree &tree, QObject *parent)
    : QAbstractItemModel(parent), m_tree(tree)
{
}

void TaskTreeModel::setBucketFilter(const QString &bucket)
{
    beginResetModel();
    m_bucketFilter = bucket.toLower();
    rebuild();
    endResetModel();
}

void TaskTreeModel::setProgressBasis(const QString &basis)
{
    m_progressBasis = basis;
    emit dataChanged(index(0, 0), index(rowCount() - 1, 0));
}

void TaskTreeModel::refresh()
{
    beginResetModel();
    rebuild();
    endResetModel();
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

    const auto tasks = m_tree.tasksInBucket(bucket);
    for (const TaskNode *node : tasks) {
        FlatRow row;
        row.tree = &m_tree;
        row.nodePtr = node;
        row.parentId = node->parentId;
        row.depth = 0;
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
    case Qt::DisplayRole: return node->title;
    default: return {};
    }
}

QHash<int, QByteArray> TaskTreeModel::roleNames() const
{
    return {
        {IdRole, "taskId"},
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
    };
}

} // namespace polomodoro
