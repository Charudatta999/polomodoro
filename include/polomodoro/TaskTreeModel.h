#pragma once

#include <QAbstractItemModel>
#include <memory>

#include "polomodoro/TaskTree.h"

namespace polomodoro {

class TaskTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        StatusRole,
        IsActiveRole,
        SelfElapsedMsRole,
        LiveElapsedMsRole,
        TotalElapsedMsRole,
        ActiveSinceRole,
        ScheduledStartAtRole,
        ScheduledEndAtRole,
        TargetMsRole,
        ProgressRatioRole,
        RemainingMsRole,
        HasTargetRole,
        TargetReachedRole,
        ListBucketRole,
        DepthRole,
        ProgressLabelRole,
        OverflowRatioRole,
        OverTargetRole,
        BadgeTextRole,
        StartableRole,
        OverdueRole,
        StartsAtLabelRole,
        HasChildrenRole,
        HasPrevSiblingRole
    };
    Q_ENUM(Roles)

    explicit TaskTreeModel(TaskTree &tree, QObject *parent = nullptr);

    void setBucketFilter(const QString &bucket);
    void setActiveSubtreeFilter(bool enabled);
    void setProgressBasis(const QString &basis);
    void setShowCompleted(bool enabled);
    void refresh();
    void tick();

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct FlatRow {
        const TaskTree *tree = nullptr;
        const void *nodePtr = nullptr;
        QString parentId;
        int depth = 0;
        bool hasPrevSibling = false;
    };

    QVector<FlatRow> m_rows;
    TaskTree &m_tree;
    QString m_bucketFilter = QStringLiteral("all");
    bool m_activeSubtree = false;
    bool m_showCompleted = false;
    QString m_progressBasis = QStringLiteral("active");

    void rebuild();
    bool rowMatchesBucket(const TaskNode &node) const;
};

} // namespace polomodoro
