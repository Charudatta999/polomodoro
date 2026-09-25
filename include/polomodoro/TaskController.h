#pragma once

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <memory>

#include "polomodoro/TaskTreeModel.h"

namespace polomodoro {

class TaskTree;
class SettingsStore;

class TaskController : public QObject {
    Q_OBJECT
    Q_PROPERTY(TaskTreeModel *model READ model CONSTANT)
    Q_PROPERTY(TaskTreeModel *activeSubtreeProxy READ activeSubtreeProxy CONSTANT)
    Q_PROPERTY(double overallProgressRatio READ overallProgressRatio NOTIFY tasksChanged)
    Q_PROPERTY(double overallRatio READ overallRatio NOTIFY tasksChanged)
    Q_PROPERTY(qint64 overallTargetMs READ overallTargetMs NOTIFY tasksChanged)
    Q_PROPERTY(int activeTaskCount READ activeTaskCount NOTIFY tasksChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY tasksChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY tasksChanged)
    Q_PROPERTY(int futureCount READ futureCount NOTIFY tasksChanged)
    Q_PROPERTY(int activeOverflowCount READ activeOverflowCount NOTIFY tasksChanged)
    Q_PROPERTY(QVariantList activeChips READ activeChips NOTIFY tasksChanged)
    Q_PROPERTY(QString combinedActiveLabel READ combinedActiveLabel NOTIFY tasksChanged)
    Q_PROPERTY(QVariant soleTargetedActiveTask READ soleTargetedActiveTask NOTIFY tasksChanged)
    Q_PROPERTY(QVariantList parentChoices READ parentChoices NOTIFY tasksChanged)
    Q_PROPERTY(bool menuOpen READ menuOpen WRITE setMenuOpen NOTIFY menuOpenChanged)
    Q_PROPERTY(bool showCompleted READ showCompleted WRITE setShowCompleted NOTIFY tasksChanged)
public:
    TaskController(TaskTree &tree, SettingsStore &settings, QObject *parent = nullptr);
    ~TaskController();

    TaskTreeModel *model() const;
    TaskTreeModel *activeSubtreeProxy() const;
    double overallProgressRatio() const;
    double overallRatio() const;
    qint64 overallTargetMs() const;
    int activeTaskCount() const;
    int activeCount() const;
    int pendingCount() const;
    int futureCount() const;
    int activeOverflowCount() const;
    QVariantList activeChips() const;
    QString combinedActiveLabel() const;
    QVariant soleTargetedActiveTask() const;
    QVariantList parentChoices() const;
    bool menuOpen() const;
    bool showCompleted() const;

    Q_INVOKABLE TaskTreeModel *proxyFor(const QString &bucket);
    Q_INVOKABLE void setMenuOpen(bool open);
    Q_INVOKABLE void setBucketFilter(const QString &bucket);
    Q_INVOKABLE void setProgressBasis(const QString &basis);
    Q_INVOKABLE void setShowCompleted(bool value);
    Q_INVOKABLE QString createTask(const QString &title, const QString &parentId = {});
    Q_INVOKABLE void startTask(const QString &id);
    Q_INVOKABLE void pauseTask(const QString &id);
    Q_INVOKABLE void resumeTask(const QString &id);
    Q_INVOKABLE void stopTask(const QString &id);
    Q_INVOKABLE void completeTask(const QString &id);
    Q_INVOKABLE void deleteTask(const QString &id);
    Q_INVOKABLE void promote(const QString &id);
    Q_INVOKABLE void demote(const QString &id);
    Q_INVOKABLE void requestEdit(const QString &id);
    Q_INVOKABLE void requestCreate(const QString &parentId);
    Q_INVOKABLE void loadInto(QObject *editor, const QString &id);
    Q_INVOKABLE void save(const QVariantMap &data);
    Q_INVOKABLE void openDrawerOnActive();
    Q_INVOKABLE void updateTaskTitle(const QString &id, const QString &title);
    Q_INVOKABLE void updateTaskTargetMs(const QString &id, qint64 targetMs);
    Q_INVOKABLE void updateTaskSchedule(const QString &id, const QString &startIso, const QString &endIso);
    Q_INVOKABLE QString formatDuration(qint64 ms) const;
    Q_INVOKABLE void refresh();

signals:
    void tasksChanged();
    void menuOpenChanged();
    void targetReached(const QString &taskId, const QString &title);
    void taskStarted(const QString &taskId, const QString &title);
    void deadlineApproaching(const QString &taskId, const QString &title);
    void editRequested(const QString &taskId);
    void createRequested(const QString &parentId);

private:
    void refreshAllModels();
    void checkTargets();
    void checkDeadlines();

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
