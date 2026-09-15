#pragma once

#include <QObject>
#include <memory>

namespace polomodoro {

class TimerEngine;
class SettingsStore;

class TimerController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(qint64 remainingMs READ remainingMs NOTIFY timeChanged)
    Q_PROPERTY(qint64 elapsedMs READ elapsedMs NOTIFY timeChanged)
    Q_PROPERTY(QString formattedTime READ formattedTime NOTIFY timeChanged)
public:
    TimerController(TimerEngine &engine, SettingsStore &settings, QObject *parent = nullptr);
    ~TimerController();

    QString mode() const;
    QString phase() const;
    bool isRunning() const;
    qint64 remainingMs() const;
    qint64 elapsedMs() const;
    QString formattedTime() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void skipPhase();
    Q_INVOKABLE void setMode(const QString &mode);

signals:
    void modeChanged();
    void phaseChanged();
    void isRunningChanged();
    void timeChanged();
    void workSegmentCompleted(qint64 durationMs);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
