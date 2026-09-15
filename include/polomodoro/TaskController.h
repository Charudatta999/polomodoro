#pragma once

#include <QObject>
#include <memory>

#include "polomodoro/TaskTreeModel.h"

namespace polomodoro {

class TaskTree;
class SettingsStore;

class TaskController : public QObject {
    Q_OBJECT
    Q_PROPERTY(TaskTreeModel *model READ model CONSTANT)
    Q_PROPERTY(double overallProgressRatio READ overallProgressRatio NOTIFY tasksChanged)
    Q_PROPERTY(int activeTaskCount READ activeTaskCount NOTIFY tasksChanged)
    Q_PROPERTY(bool menuOpen READ menuOpen WRITE setMenuOpen NOTIFY menuOpenChanged)
public:
    TaskController(TaskTree &tree, SettingsStore &settings, QObject *parent = nullptr);
    ~TaskController();

    TaskTreeModel *model() const;
    double overallProgressRatio() const;
    int activeTaskCount() const;
    bool menuOpen() const;

    Q_INVOKABLE void setMenuOpen(bool open);
    Q_INVOKABLE void setBucketFilter(const QString &bucket);
    Q_INVOKABLE void setProgressBasis(const QString &basis);
    Q_INVOKABLE QString createTask(const QString &title, const QString &parentId = {});
    Q_INVOKABLE void startTask(const QString &id);
    Q_INVOKABLE void pauseTask(const QString &id);
    Q_INVOKABLE void resumeTask(const QString &id);
    Q_INVOKABLE void stopTask(const QString &id);
    Q_INVOKABLE void completeTask(const QString &id);
    Q_INVOKABLE void deleteTask(const QString &id);
    Q_INVOKABLE void updateTaskTitle(const QString &id, const QString &title);
    Q_INVOKABLE void updateTaskTargetMs(const QString &id, qint64 targetMs);
    Q_INVOKABLE void updateTaskSchedule(const QString &id, const QString &startIso, const QString &endIso);
    Q_INVOKABLE QString formatDuration(qint64 ms) const;
    Q_INVOKABLE void refresh();

signals:
    void tasksChanged();
    void menuOpenChanged();
    void targetReached(const QString &taskId, const QString &title);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
