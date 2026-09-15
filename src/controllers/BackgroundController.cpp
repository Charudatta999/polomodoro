#include "polomodoro/BackgroundController.h"

#include "polomodoro/BackgroundManager.h"
#include "polomodoro/SettingsStore.h"

namespace polomodoro {

struct BackgroundController::Impl {
    BackgroundManager &manager;
    SettingsStore &settings;
    QString phaseTint = QStringLiteral("neutral");
};

BackgroundController::BackgroundController(BackgroundManager &manager, SettingsStore &settings, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>(Impl{manager, settings}))
{
}

BackgroundController::~BackgroundController() = default;

QString BackgroundController::currentImageUrl() const { return d->manager.currentImageUrl(); }
QString BackgroundController::previousImageUrl() const { return d->manager.previousImageUrl(); }
QString BackgroundController::source() const { return backgroundSource(); }
QString BackgroundController::backgroundSource() const { return d->manager.backgroundSource(); }
QString BackgroundController::phaseTint() const { return d->phaseTint; }

void BackgroundController::setSource(const QString &source)
{
    setBackgroundSource(source);
}

void BackgroundController::setBackgroundSource(const QString &source)
{
    d->manager.setBackgroundSource(source);
    d->settings.setString(QStringLiteral("backgroundSource"), source);
    emit backgroundChanged();
}

void BackgroundController::setPhaseTint(const QString &tint)
{
    d->phaseTint = tint;
    emit backgroundChanged();
}

void BackgroundController::cycleSource()
{
    cycleBackgroundSource();
}

void BackgroundController::cycleBackgroundSource()
{
    const QString next = backgroundSource() == QStringLiteral("wallpaper") ? QStringLiteral("spotify") : QStringLiteral("wallpaper");
    setBackgroundSource(next);
}

void BackgroundController::tick()
{
    d->manager.tick();
    emit backgroundChanged();
}

} // namespace polomodoro
