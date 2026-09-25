#include "polomodoro/TimerController.h"

#include "polomodoro/SettingsStore.h"
#include "polomodoro/TimerEngine.h"

#include <QLoggingCategory>
#include <QTimer>

namespace polomodoro {

Q_LOGGING_CATEGORY(lcTimer, "polomodoro.timer")

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
        if (before == PomodoroPhase::Work && d->engine->phase() != PomodoroPhase::Work && d->segmentElapsed > 0) {
            qCInfo(lcTimer) << "work segment complete" << d->segmentElapsed << "ms → phase" << int(d->engine->phase());
            emit workSegmentCompleted(d->segmentElapsed);
        }
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
    if (d->engine->mode() == TimerMode::Stopwatch)
        return QStringLiteral("stopwatch");
    switch (d->engine->phase()) {
    case PomodoroPhase::Work: return QStringLiteral("work");
    case PomodoroPhase::ShortBreak: return QStringLiteral("shortBreak");
    case PomodoroPhase::LongBreak: return QStringLiteral("longBreak");
    }
    return QStringLiteral("work");
}

QString TimerController::phaseLabel() const
{
    if (d->engine->mode() == TimerMode::Stopwatch)
        return QStringLiteral("Stopwatch");
    switch (d->engine->phase()) {
    case PomodoroPhase::Work: return QStringLiteral("Focus");
    case PomodoroPhase::ShortBreak: return QStringLiteral("Short break");
    case PomodoroPhase::LongBreak: return QStringLiteral("Long break");
    }
    return QStringLiteral("Focus");
}

int TimerController::cycleIndex() const
{
    const int cycles = d->settings->getInt(QStringLiteral("pomodoroCyclesBeforeLongBreak"), 4);
    if (cycles <= 0)
        return 0;
    return d->engine->completedWorkCycles() % cycles;
}

QString TimerController::cycleLabel() const
{
    if (d->engine->mode() == TimerMode::Stopwatch)
        return QStringLiteral("Stopwatch mode");
    const int cycles = d->settings->getInt(QStringLiteral("pomodoroCyclesBeforeLongBreak"), 4);
    int nextBreakMin = d->settings->getInt(QStringLiteral("pomodoroShortBreakMs"), 300000) / 60000;
    if (d->engine->phase() == PomodoroPhase::Work) {
        const bool longNext = ((d->engine->completedWorkCycles() + 1) % cycles) == 0;
        nextBreakMin = longNext ? d->settings->getInt(QStringLiteral("pomodoroLongBreakMs"), 900000) / 60000
                                : d->settings->getInt(QStringLiteral("pomodoroShortBreakMs"), 300000) / 60000;
    } else if (d->engine->phase() == PomodoroPhase::LongBreak) {
        nextBreakMin = d->settings->getInt(QStringLiteral("pomodoroLongBreakMs"), 900000) / 60000;
    }
    return QStringLiteral("Cycle %1 of %2 · next break %3m")
        .arg(cycleIndex() + 1)
        .arg(cycles)
        .arg(nextBreakMin);
}

bool TimerController::isRunning() const { return d->engine->isRunning(); }
bool TimerController::isPaused() const { return !d->engine->isRunning() && d->engine->wasStarted(); }

qreal TimerController::progress() const
{
    const qint64 duration = d->engine->phaseDurationMs();
    if (duration <= 0)
        return 0.0;
    if (d->engine->mode() == TimerMode::Stopwatch)
        return qreal(d->engine->elapsedMs() % duration) / qreal(duration);
    const qint64 elapsed = duration - d->engine->remainingMs();
    return qBound(0.0, qreal(elapsed) / qreal(duration), 1.0);
}

static QString twoDigits(qint64 value)
{
    return QStringLiteral("%1").arg(value, 2, 10, QChar('0'));
}

QString TimerController::minutes() const
{
    const qint64 ms = d->engine->mode() == TimerMode::Pomodoro ? d->engine->remainingMs() : d->engine->elapsedMs();
    return QString::number(qMax<qint64>(0, (ms / 1000) / 60));
}

QString TimerController::seconds() const
{
    const qint64 ms = d->engine->mode() == TimerMode::Pomodoro ? d->engine->remainingMs() : d->engine->elapsedMs();
    return twoDigits(qMax<qint64>(0, (ms / 1000) % 60));
}

qint64 TimerController::remainingMs() const { return d->engine->remainingMs(); }
qint64 TimerController::elapsedMs() const { return d->engine->elapsedMs(); }
QString TimerController::formattedTime() const { return d->engine->formattedTime(); }

void TimerController::start()
{
    qCInfo(lcTimer) << "start" << mode() << phase() << formattedTime();
    d->engine->start();
    d->tick.start();
    emit isRunningChanged();
}

void TimerController::pause()
{
    qCInfo(lcTimer) << "pause" << mode() << phase() << formattedTime();
    d->engine->pause();
    d->tick.stop();
    emit isRunningChanged();
}

void TimerController::toggle()
{
    if (isRunning())
        pause();
    else
        start();
}

void TimerController::reset()
{
    qCInfo(lcTimer) << "reset";
    d->engine->reset();
    d->segmentElapsed = 0;
    emit timeChanged();
    emit phaseChanged();
    emit isRunningChanged();
}

void TimerController::skipPhase()
{
    qCInfo(lcTimer) << "skipPhase from" << phase();
    d->engine->skipPhase();
    d->segmentElapsed = 0;
    emit phaseChanged();
    emit timeChanged();
}

void TimerController::toggleMode()
{
    setMode(mode() == QStringLiteral("pomodoro") ? QStringLiteral("stopwatch") : QStringLiteral("pomodoro"));
}

void TimerController::setMode(const QString &mode)
{
    qCInfo(lcTimer) << "setMode" << this->mode() << "→" << mode;
    d->engine->setMode(mode == QStringLiteral("stopwatch") ? TimerMode::Stopwatch : TimerMode::Pomodoro);
    d->segmentElapsed = 0;
    emit modeChanged();
    emit phaseChanged();
    emit timeChanged();
    emit isRunningChanged();
}

} // namespace polomodoro
