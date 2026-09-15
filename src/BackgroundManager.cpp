#include "polomodoro/BackgroundManager.h"

#include "polomodoro/SettingsStore.h"

#include <QDir>
#include <QStandardPaths>
#include <QUrl>

namespace polomodoro {

struct BackgroundManager::Impl {
    SettingsStore &settings;
    QStringList wallpaperPaths;
    int wallpaperIndex = 0;
    qint64 lastRotationMs = 0;
    QString spotifyArtUrl;
    QString currentUrl;
    QString previousUrl;
};

BackgroundManager::BackgroundManager(SettingsStore &settings)
    : d(std::make_unique<Impl>(Impl{settings}))
{
    const QString userPath = settings.getString(QStringLiteral("backgroundUserPath"));
    if (!userPath.isEmpty()) {
        const QDir dir(userPath);
        const auto files = dir.entryList({QStringLiteral("*.jpg"), QStringLiteral("*.png"), QStringLiteral("*.webp")}, QDir::Files);
        for (const QString &f : files)
            d->wallpaperPaths.push_back(dir.filePath(f));
    }
    if (d->wallpaperPaths.isEmpty())
        d->currentUrl = QStringLiteral("qrc:/wallpapers/default.png");
    else
        d->currentUrl = QUrl::fromLocalFile(d->wallpaperPaths.first()).toString();
}

BackgroundManager::~BackgroundManager() = default;

QString BackgroundManager::currentImageUrl() const
{
    if (backgroundSource() == QStringLiteral("spotify") && !d->spotifyArtUrl.isEmpty())
        return d->spotifyArtUrl;
    return d->currentUrl;
}

QString BackgroundManager::previousImageUrl() const
{
    return d->previousUrl;
}

QString BackgroundManager::backgroundSource() const
{
    return d->settings.getString(QStringLiteral("backgroundSource"), QStringLiteral("wallpaper"));
}

void BackgroundManager::setBackgroundSource(const QString &source)
{
    d->settings.setString(QStringLiteral("backgroundSource"), source);
}

void BackgroundManager::setSpotifyArtUrl(const QString &url)
{
    d->spotifyArtUrl = url;
}

void BackgroundManager::tick()
{
    if (backgroundSource() != QStringLiteral("wallpaper") || d->wallpaperPaths.isEmpty())
        return;
    const int rotationSec = d->settings.getInt(QStringLiteral("backgroundRotationSec"), 300);
    d->lastRotationMs += 1000;
    if (d->lastRotationMs < rotationSec * 1000LL)
        return;
    d->lastRotationMs = 0;
    d->previousUrl = d->currentUrl;
    d->wallpaperIndex = (d->wallpaperIndex + 1) % d->wallpaperPaths.size();
    d->currentUrl = QUrl::fromLocalFile(d->wallpaperPaths.at(d->wallpaperIndex)).toString();
}

} // namespace polomodoro
