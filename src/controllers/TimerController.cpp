#include "polomodoro/TimerController.h"

#include "polomodoro/SettingsStore.h"
#include "polomodoro/TimerEngine.h"

#include <QTimer>

namespace polomodoro {

struct TimerController::Impl {
    TimerEngine *engine = nullptr;
    SettingsStore *settings = nullptr;
    QTimer tick;
    qint64 segmentElapsed = 0;
};

TimerController::TimerController(TimerEngine &engine, SettingsStore &settings, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>())
{
    d->engine = &engine;
    d->settings = &settings;
    d->engine->setPomodoroDurations(
        settings.getInt(QStringLiteral("pomodoroWorkMs"), 1500000),
        settings.getInt(QStringLiteral("pomodoroShortBreakMs"), 300000),
        settings.getInt(QStringLiteral("pomodoroLongBreakMs"), 900000),
        settings.getInt(QStringLiteral("pomodoroCyclesBeforeLongBreak"), 4));

    d->tick.setInterval(100);
    connect(&d->tick, &QTimer::timeout, this, [this]() {
        const PomodoroPhase before = d->engine->phase();
        d->engine->tick(100);
        if (before == PomodoroPhase::Work && d->engine->phase() != PomodoroPhase::Work && d->segmentElapsed > 0)
            emit workSegmentCompleted(d->segmentElapsed);
        if (d->engine->phase() == PomodoroPhase::Work && d->engine->isRunning())
            d->segmentElapsed += 100;
        emit timeChanged();
    });
}

TimerController::~TimerController() = default;

QString TimerController::mode() const
{
    return d->engine->mode() == TimerMode::Pomodoro ? QStringLiteral("pomodoro") : QStringLiteral("stopwatch");
}

QString TimerController::phase() const
{
    switch (d->engine->phase()) {
    case PomodoroPhase::Work: return QStringLiteral("work");
    case PomodoroPhase::ShortBreak: return QStringLiteral("shortBreak");
    case PomodoroPhase::LongBreak: return QStringLiteral("longBreak");
    }
    return QStringLiteral("work");
}

bool TimerController::isRunning() const { return d->engine->isRunning(); }
qint64 TimerController::remainingMs() const { return d->engine->remainingMs(); }
qint64 TimerController::elapsedMs() const { return d->engine->elapsedMs(); }
QString TimerController::formattedTime() const { return d->engine->formattedTime(); }

void TimerController::start()
{
    d->engine->start();
    d->tick.start();
    emit isRunningChanged();
}

void TimerController::pause()
{
    d->engine->pause();
    d->tick.stop();
    emit isRunningChanged();
}

void TimerController::reset()
{
    d->engine->reset();
    d->segmentElapsed = 0;
    emit timeChanged();
    emit phaseChanged();
}

void TimerController::skipPhase()
{
    d->engine->skipPhase();
    d->segmentElapsed = 0;
    emit phaseChanged();
    emit timeChanged();
}

void TimerController::setMode(const QString &mode)
{
    d->engine->setMode(mode == QStringLiteral("stopwatch") ? TimerMode::Stopwatch : TimerMode::Pomodoro);
    d->segmentElapsed = 0;
    emit modeChanged();
    emit timeChanged();
}

} // namespace polomodoro
