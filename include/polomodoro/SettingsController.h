#pragma once

#include <QObject>
#include <memory>

namespace polomodoro {

class SettingsStore;

class SettingsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool alwaysOnTop READ alwaysOnTop WRITE setAlwaysOnTop NOTIFY settingsChanged)
    Q_PROPERTY(QString backgroundSource READ backgroundSource WRITE setBackgroundSource NOTIFY settingsChanged)
    Q_PROPERTY(int backgroundRotationSec READ backgroundRotationSec WRITE setBackgroundRotationSec NOTIFY settingsChanged)
    Q_PROPERTY(bool notifyOnTargetReached READ notifyOnTargetReached WRITE setNotifyOnTargetReached NOTIFY settingsChanged)
public:
    explicit SettingsController(SettingsStore &store, QObject *parent = nullptr);
    ~SettingsController();

    bool alwaysOnTop() const;
    QString backgroundSource() const;
    int backgroundRotationSec() const;
    bool notifyOnTargetReached() const;

    void setAlwaysOnTop(bool value);
    void setBackgroundSource(const QString &value);
    void setBackgroundRotationSec(int value);
    void setNotifyOnTargetReached(bool value);

    Q_INVOKABLE int pomodoroWorkMs() const;
    Q_INVOKABLE int pomodoroShortBreakMs() const;
    Q_INVOKABLE int pomodoroLongBreakMs() const;

signals:
    void settingsChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
