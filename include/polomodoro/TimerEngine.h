#pragma once

#include <memory>
#include <QString>

namespace polomodoro {

enum class TimerMode { Pomodoro, Stopwatch };
enum class PomodoroPhase { Work, ShortBreak, LongBreak };

class TimerEngine {
public:
    TimerEngine();
    ~TimerEngine();

    TimerMode mode() const;
    PomodoroPhase phase() const;
    bool isRunning() const;
    bool wasStarted() const;
    qint64 remainingMs() const;
    qint64 elapsedMs() const;
    qint64 phaseDurationMs() const;
    int completedWorkCycles() const;

    void setMode(TimerMode mode);
    void setPomodoroDurations(qint64 workMs, qint64 shortBreakMs, qint64 longBreakMs, int cyclesBeforeLongBreak);

    void start();
    void pause();
    void reset();
    void skipPhase();
    void tick(qint64 deltaMs);

    QString formattedTime() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
