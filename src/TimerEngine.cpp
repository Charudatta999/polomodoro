#include "polomodoro/TimerEngine.h"

#include <QtGlobal>

namespace polomodoro {

struct TimerEngine::Impl {
    TimerMode mode = TimerMode::Pomodoro;
    PomodoroPhase phase = PomodoroPhase::Work;
    bool running = false;
    qint64 remainingMs = 25 * 60 * 1000;
    qint64 elapsedMs = 0;
    qint64 workMs = 25 * 60 * 1000;
    qint64 shortBreakMs = 5 * 60 * 1000;
    qint64 longBreakMs = 15 * 60 * 1000;
    int cyclesBeforeLongBreak = 4;
    int completedWorkCycles = 0;
    bool wasStarted = false;
};

TimerEngine::TimerEngine() : d(std::make_unique<Impl>()) {}
TimerEngine::~TimerEngine() = default;

TimerMode TimerEngine::mode() const { return d->mode; }
PomodoroPhase TimerEngine::phase() const { return d->phase; }
bool TimerEngine::isRunning() const { return d->running; }
bool TimerEngine::wasStarted() const { return d->wasStarted; }
qint64 TimerEngine::remainingMs() const { return d->remainingMs; }
qint64 TimerEngine::elapsedMs() const { return d->elapsedMs; }
int TimerEngine::completedWorkCycles() const { return d->completedWorkCycles; }

qint64 TimerEngine::phaseDurationMs() const
{
    if (d->mode == TimerMode::Stopwatch)
        return 60 * 60 * 1000;
    switch (d->phase) {
    case PomodoroPhase::Work: return d->workMs;
    case PomodoroPhase::ShortBreak: return d->shortBreakMs;
    case PomodoroPhase::LongBreak: return d->longBreakMs;
    }
    return d->workMs;
}

void TimerEngine::setMode(TimerMode mode)
{
    d->mode = mode;
    reset();
}

void TimerEngine::setPomodoroDurations(qint64 workMs, qint64 shortBreakMs, qint64 longBreakMs, int cyclesBeforeLongBreak)
{
    d->workMs = workMs;
    d->shortBreakMs = shortBreakMs;
    d->longBreakMs = longBreakMs;
    d->cyclesBeforeLongBreak = cyclesBeforeLongBreak;
    if (d->mode == TimerMode::Pomodoro && d->phase == PomodoroPhase::Work)
        d->remainingMs = workMs;
}

void TimerEngine::start()
{
    d->running = true;
    d->wasStarted = true;
}
void TimerEngine::pause() { d->running = false; }

void TimerEngine::reset()
{
    d->running = false;
    d->wasStarted = false;
    d->elapsedMs = 0;
    d->completedWorkCycles = 0;
    d->phase = PomodoroPhase::Work;
    d->remainingMs = d->mode == TimerMode::Pomodoro ? d->workMs : 0;
}

void TimerEngine::skipPhase()
{
    if (d->mode != TimerMode::Pomodoro)
        return;
    if (d->phase == PomodoroPhase::Work) {
        d->completedWorkCycles++;
        if (d->completedWorkCycles % d->cyclesBeforeLongBreak == 0)
            d->phase = PomodoroPhase::LongBreak;
        else
            d->phase = PomodoroPhase::ShortBreak;
        d->remainingMs = d->phase == PomodoroPhase::LongBreak ? d->longBreakMs : d->shortBreakMs;
    } else {
        d->phase = PomodoroPhase::Work;
        d->remainingMs = d->workMs;
    }
}

void TimerEngine::tick(qint64 deltaMs)
{
    if (!d->running)
        return;

    if (d->mode == TimerMode::Stopwatch) {
        d->elapsedMs += deltaMs;
        return;
    }

    d->remainingMs -= deltaMs;
    if (d->phase == PomodoroPhase::Work)
        d->elapsedMs += deltaMs;

    if (d->remainingMs > 0)
        return;

    const qint64 overflow = -d->remainingMs;
    if (d->phase == PomodoroPhase::Work) {
        d->completedWorkCycles++;
        if (d->completedWorkCycles % d->cyclesBeforeLongBreak == 0) {
            d->phase = PomodoroPhase::LongBreak;
            d->remainingMs = d->longBreakMs - overflow;
        } else {
            d->phase = PomodoroPhase::ShortBreak;
            d->remainingMs = d->shortBreakMs - overflow;
        }
    } else {
        d->phase = PomodoroPhase::Work;
        d->remainingMs = d->workMs - overflow;
        d->elapsedMs = 0;
    }
}

QString TimerEngine::formattedTime() const
{
    qint64 ms = d->mode == TimerMode::Pomodoro ? d->remainingMs : d->elapsedMs;
    if (ms < 0)
        ms = 0;
    const qint64 totalSec = ms / 1000;
    const qint64 min = totalSec / 60;
    const qint64 sec = totalSec % 60;
    return QStringLiteral("%1:%2")
        .arg(min, 2, 10, QChar('0'))
        .arg(sec, 2, 10, QChar('0'));
}

} // namespace polomodoro
