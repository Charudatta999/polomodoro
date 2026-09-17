#include "polomodoro/BackgroundController.h"

#include "polomodoro/BackgroundManager.h"
#include "polomodoro/SettingsStore.h"

#include <QLoggingCategory>
#include <QUrl>

namespace polomodoro {

Q_LOGGING_CATEGORY(lcBgCtl, "polomodoro.background")

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
bool BackgroundController::firstRunAsked() const
{
    return d->settings.getBool(QStringLiteral("backgroundFirstRunAsked"));
}

void BackgroundController::setUserFolder(const QUrl &folder)
{
    qCInfo(lcBgCtl) << "setUserFolder" << folder;
    d->manager.setUserWallpaperFolder(folder.toLocalFile());
    d->settings.setBool(QStringLiteral("backgroundFirstRunAsked"), true);
    qCInfo(lcBgCtl) << "showing" << currentImageUrl();
    emit backgroundChanged();
}

void BackgroundController::markFirstRunAsked()
{
    qCInfo(lcBgCtl) << "first-run wallpaper dialog dismissed; asked=" << true;
    d->settings.setBool(QStringLiteral("backgroundFirstRunAsked"), true);
    emit backgroundChanged();
}

void BackgroundController::setSource(const QString &source)
{
    setBackgroundSource(source);
}

void BackgroundController::setBackgroundSource(const QString &source)
{
    qCInfo(lcBgCtl) << "source" << backgroundSource() << "→" << source;
    d->manager.setBackgroundSource(source);
    d->settings.setString(QStringLiteral("backgroundSource"), source);
    qCInfo(lcBgCtl) << "showing" << currentImageUrl();
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
    if (d->manager.tick()) {
        qCInfo(lcBgCtl) << "tick rotated to" << currentImageUrl();
        emit backgroundChanged();
    }
}

} // namespace polomodoro
