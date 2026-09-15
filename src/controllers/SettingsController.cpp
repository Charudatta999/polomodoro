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
int SettingsController::cyclesBeforeLongBreak() const { return d->store.getInt(QStringLiteral("pomodoroCyclesBeforeLongBreak"), 4); }
bool SettingsController::showDayTimeline() const { return d->store.getBool(QStringLiteral("showDayTimeline"), true); }
bool SettingsController::barDropdownExpanded() const { return d->store.getBool(QStringLiteral("barDropdownExpanded")); }
int SettingsController::workMinutes() const { return pomodoroWorkMs() / 60000; }
int SettingsController::shortBreakMinutes() const { return pomodoroShortBreakMs() / 60000; }
int SettingsController::longBreakMinutes() const { return pomodoroLongBreakMs() / 60000; }
int SettingsController::rotationSec() const { return backgroundRotationSec(); }
QString SettingsController::progressBasis() const { return d->store.getString(QStringLiteral("targetProgressBasis"), QStringLiteral("active")); }
QString SettingsController::accentMode() const { return d->store.getString(QStringLiteral("accentMode"), QStringLiteral("auto")); }
QString SettingsController::accentManualPalette() const { return d->store.getString(QStringLiteral("accentManualPalette"), QStringLiteral("forest")); }
QString SettingsController::integrityReport() const { return {}; }

QVariantList SettingsController::palettePresets() const
{
    return {
        QVariantMap{{QStringLiteral("id"), QStringLiteral("forest")}, {QStringLiteral("accent"), QStringLiteral("#1ED760")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("ocean")}, {QStringLiteral("accent"), QStringLiteral("#58C8B0")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("ember")}, {QStringLiteral("accent"), QStringLiteral("#E8C547")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("rose")}, {QStringLiteral("accent"), QStringLiteral("#E8489B")}},
    };
}

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

void SettingsController::setShowDayTimeline(bool value)
{
    d->store.setBool(QStringLiteral("showDayTimeline"), value);
    emit settingsChanged();
}

void SettingsController::setBarDropdownExpanded(bool value)
{
    d->store.setBool(QStringLiteral("barDropdownExpanded"), value);
    emit settingsChanged();
}

void SettingsController::setWorkMinutes(int value)
{
    d->store.setInt(QStringLiteral("pomodoroWorkMs"), value * 60000);
    emit settingsChanged();
}

void SettingsController::setShortBreakMinutes(int value)
{
    d->store.setInt(QStringLiteral("pomodoroShortBreakMs"), value * 60000);
    emit settingsChanged();
}

void SettingsController::setLongBreakMinutes(int value)
{
    d->store.setInt(QStringLiteral("pomodoroLongBreakMs"), value * 60000);
    emit settingsChanged();
}

void SettingsController::setRotationSec(int value)
{
    setBackgroundRotationSec(value);
}

void SettingsController::setProgressBasis(const QString &value)
{
    d->store.setString(QStringLiteral("targetProgressBasis"), value);
    emit settingsChanged();
}

void SettingsController::setAccentMode(const QString &value)
{
    d->store.setString(QStringLiteral("accentMode"), value);
    emit settingsChanged();
}

void SettingsController::setAccentManualPalette(const QString &value)
{
    d->store.setString(QStringLiteral("accentManualPalette"), value);
    emit settingsChanged();
}

int SettingsController::pomodoroWorkMs() const { return d->store.getInt(QStringLiteral("pomodoroWorkMs"), 1500000); }
int SettingsController::pomodoroShortBreakMs() const { return d->store.getInt(QStringLiteral("pomodoroShortBreakMs"), 300000); }
int SettingsController::pomodoroLongBreakMs() const { return d->store.getInt(QStringLiteral("pomodoroLongBreakMs"), 900000); }

void SettingsController::exportCsv()
{
    // Phase 4: export sessions to ~/Documents
}

} // namespace polomodoro
