#include "polomodoro/SettingsController.h"

#include "polomodoro/SettingsStore.h"

namespace polomodoro {

struct SettingsController::Impl { SettingsStore &store; };

SettingsController::SettingsController(SettingsStore &store, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>(Impl{store}))
{
}

SettingsController::~SettingsController() = default;

bool SettingsController::alwaysOnTop() const { return d->store.getBool(QStringLiteral("alwaysOnTop")); }
QString SettingsController::backgroundSource() const { return d->store.getString(QStringLiteral("backgroundSource")); }
int SettingsController::backgroundRotationSec() const { return d->store.getInt(QStringLiteral("backgroundRotationSec"), 300); }
bool SettingsController::notifyOnTargetReached() const { return d->store.getBool(QStringLiteral("notifyOnTargetReached")); }

void SettingsController::setAlwaysOnTop(bool value)
{
    d->store.setBool(QStringLiteral("alwaysOnTop"), value);
    emit settingsChanged();
}

void SettingsController::setBackgroundSource(const QString &value)
{
    d->store.setString(QStringLiteral("backgroundSource"), value);
    emit settingsChanged();
}

void SettingsController::setBackgroundRotationSec(int value)
{
    d->store.setInt(QStringLiteral("backgroundRotationSec"), value);
    emit settingsChanged();
}

void SettingsController::setNotifyOnTargetReached(bool value)
{
    d->store.setBool(QStringLiteral("notifyOnTargetReached"), value);
    emit settingsChanged();
}

int SettingsController::pomodoroWorkMs() const { return d->store.getInt(QStringLiteral("pomodoroWorkMs"), 1500000); }
int SettingsController::pomodoroShortBreakMs() const { return d->store.getInt(QStringLiteral("pomodoroShortBreakMs"), 300000); }
int SettingsController::pomodoroLongBreakMs() const { return d->store.getInt(QStringLiteral("pomodoroLongBreakMs"), 900000); }

} // namespace polomodoro
