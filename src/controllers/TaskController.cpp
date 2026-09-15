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
    QTimer liveTimer;
    bool menuOpen = false;
    QString progressBasis;
};

TaskController::TaskController(TaskTree &tree, SettingsStore &settings, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>())
{
    d->tree = &tree;
    d->settings = &settings;
    d->progressBasis = d->settings->getString(QStringLiteral("targetProgressBasis"), QStringLiteral("active"));
    d->model = std::make_unique<TaskTreeModel>(*d->tree, this);
    d->model->setProgressBasis(d->progressBasis);

    d->liveTimer.setInterval(1000);
    connect(&d->liveTimer, &QTimer::timeout, this, [this]() {
        d->tree->promoteFutureTasks();
        d->model->refresh();
        emit tasksChanged();
    });
    d->liveTimer.start();
}

TaskController::~TaskController() = default;

TaskTreeModel *TaskController::model() const { return d->model.get(); }
double TaskController::overallProgressRatio() const { return d->tree->overallProgressRatio(d->progressBasis); }
int TaskController::activeTaskCount() const { return d->tree->activeTasks().size(); }
bool TaskController::menuOpen() const { return d->menuOpen; }

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
    d->model->setProgressBasis(basis);
}

QString TaskController::createTask(const QString &title, const QString &parentId)
{
    const QString id = d->tree->createTask(title, parentId);
    d->model->refresh();
    emit tasksChanged();
    return id;
}

void TaskController::startTask(const QString &id)
{
    d->tree->startTask(id);
    d->model->refresh();
    emit tasksChanged();
}

void TaskController::pauseTask(const QString &id)
{
    d->tree->pauseTask(id);
    d->model->refresh();
    emit tasksChanged();
}

void TaskController::resumeTask(const QString &id)
{
    d->tree->resumeTask(id);
    d->model->refresh();
    emit tasksChanged();
}

void TaskController::stopTask(const QString &id)
{
    d->tree->stopTask(id);
    d->model->refresh();
    emit tasksChanged();
}

void TaskController::completeTask(const QString &id)
{
    d->tree->completeTask(id);
    d->model->refresh();
    emit tasksChanged();
}

void TaskController::deleteTask(const QString &id)
{
    d->tree->deleteTask(id);
    d->model->refresh();
    emit tasksChanged();
}

void TaskController::updateTaskTitle(const QString &id, const QString &title)
{
    if (TaskNode *node = d->tree->findById(id)) {
        node->title = title;
        d->tree->saveTask(id);
        d->model->refresh();
        emit tasksChanged();
    }
}

void TaskController::updateTaskTargetMs(const QString &id, qint64 targetMs)
{
    if (TaskNode *node = d->tree->findById(id)) {
        node->targetMs = targetMs;
        d->tree->saveTask(id);
        d->model->refresh();
        emit tasksChanged();
    }
}

void TaskController::updateTaskSchedule(const QString &id, const QString &startIso, const QString &endIso)
{
    if (TaskNode *node = d->tree->findById(id)) {
        node->scheduledStartAt = startIso.isEmpty() ? QDateTime() : QDateTime::fromString(startIso, Qt::ISODateWithMs);
        node->scheduledEndAt = endIso.isEmpty() ? QDateTime() : QDateTime::fromString(endIso, Qt::ISODateWithMs);
        d->tree->saveTask(id);
        d->model->refresh();
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
    d->model->refresh();
    emit tasksChanged();
}

} // namespace polomodoro
