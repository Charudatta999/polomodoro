#pragma once

#include <memory>
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>

namespace polomodoro {

class DatabaseManager;

enum class TaskStatus { Idle, Active, Paused, Stopped, Completed };

struct TaskNode {
    QString id;
    QString title;
    QString parentId;
    int sortOrder = 0;
    TaskStatus status = TaskStatus::Idle;
    QDateTime activeSince;
    QDateTime scheduledStartAt;
    QDateTime scheduledEndAt;
    qint64 selfElapsedMs = 0;
    qint64 targetMs = -1;
    QDateTime targetReachedAt;
    QDateTime createdAt;
    QDateTime updatedAt;
    QVector<TaskNode> children;
};

enum class TaskListBucket { Future, Pending, Active, All };

// One logged work segment, as stored in the sessions table.
struct SessionRecord {
    qint64 id = 0;
    QString taskId;
    QDateTime startedAt;   // UTC
    qint64 durationMs = 0;
    QString mode;
};

class TaskTree {
public:
    explicit TaskTree(DatabaseManager &db);
    ~TaskTree();

    bool load();
    bool saveAll();
    bool saveTask(const QString &id);
    void heartbeat();

    const QVector<TaskNode> &roots() const;

    QString createTask(const QString &title, const QString &parentId = {});
    bool updateTask(const TaskNode &node);
    bool deleteTask(const QString &id);
    bool reparentTask(const QString &id, const QString &newParentId);
    bool promoteTask(const QString &id);
    bool demoteTask(const QString &id);

    bool startTask(const QString &id);
    bool pauseTask(const QString &id);
    bool resumeTask(const QString &id);
    bool stopTask(const QString &id);
    bool completeTask(const QString &id);

    TaskNode *findById(const QString &id);
    const TaskNode *findById(const QString &id) const;

    qint64 liveElapsedMs(const TaskNode &node) const;
    qint64 loggedWorkMs(const QString &taskId) const;
    qint64 progressMs(const TaskNode &node, const QString &basis) const;
    double progressRatio(const TaskNode &node, const QString &basis) const;

    QVector<const TaskNode *> activeTasks() const;
    QVector<const TaskNode *> tasksInBucket(TaskListBucket bucket) const;
    QVector<const TaskNode *> activeSubtree() const;

    struct FlatTreeRow {
        const TaskNode *node = nullptr;
        int depth = 0;
        bool hasPrevSibling = false;
    };
    QVector<FlatTreeRow> flattenBucket(TaskListBucket bucket, bool showCompleted) const;

    double overallProgressRatio(const QString &basis) const;

    void addSession(const QString &taskId, const QDateTime &startedAt, qint64 durationMs, const QString &mode);
    QVector<SessionRecord> sessionsBetween(const QDateTime &fromUtc, const QDateTime &toUtc) const;

    // Moves a task's scheduled window to a new start, preserving its duration.
    bool rescheduleTask(const QString &id, const QDateTime &newStartUtc);

    // Every task in the tree, depth-first. The timeline needs all of them
    // regardless of bucket.
    QVector<const TaskNode *> allTasks() const;
    void reconcileOnStartup();
    void promoteFutureTasks();

    bool anyTaskActive() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;

    void finalizeActiveSegment(TaskNode &node, const QDateTime &now);
    void recomputeTotals(TaskNode &node);
    void loadFlat();
};

} // namespace polomodoro
