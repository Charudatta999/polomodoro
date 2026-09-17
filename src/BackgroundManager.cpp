#include "polomodoro/BackgroundManager.h"

#include "polomodoro/SettingsStore.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QUrl>

namespace polomodoro {

Q_LOGGING_CATEGORY(lcBg, "polomodoro.background")

namespace {

const QStringList kBundled = {QStringLiteral("qrc:/wallpapers/default.png"),
                              QStringLiteral("qrc:/wallpapers/grad1.png"),
                              QStringLiteral("qrc:/wallpapers/grad2.png"),
                              QStringLiteral("qrc:/wallpapers/grad3.png")};

QStringList imageFilesIn(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists())
        return {};
    const auto names = dir.entryList({QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"),
                                      QStringLiteral("*.png"), QStringLiteral("*.webp"),
                                      QStringLiteral("*.JPG"), QStringLiteral("*.JPEG"),
                                      QStringLiteral("*.PNG"), QStringLiteral("*.WEBP")},
                                     QDir::Files, QDir::Name);
    QStringList out;
    out.reserve(names.size());
    for (const QString &n : names)
        out.push_back(dir.filePath(n));
    return out;
}

QString localPathFromUrl(const QString &url)
{
    if (url.startsWith(QLatin1String("qrc:")))
        return url.split(QLatin1Char('#')).constFirst();
    return QUrl(url).toLocalFile();
}

QString toDisplayUrl(const QString &path)
{
    if (path.startsWith(QLatin1String("qrc:")))
        return path;
    QUrl u = QUrl::fromLocalFile(path);
    const qint64 mtime = QFileInfo(path).lastModified().toMSecsSinceEpoch();
    if (mtime > 0)
        u.setFragment(QString::number(mtime));
    return u.toString();
}

} // namespace

struct BackgroundManager::Impl {
    SettingsStore &settings;
    QStringList wallpaperPaths;
    int wallpaperIndex = 0;
    qint64 lastRotationMs = 0;
    QString spotifyArtUrl;
    QString currentUrl;
    QString previousUrl;

    void reloadPaths()
    {
        const QString userPath = settings.getString(QStringLiteral("backgroundUserPath"));
        QStringList paths;
        if (!userPath.isEmpty())
            paths = imageFilesIn(userPath);
        if (paths.isEmpty())
            paths = kBundled;

        const QString currentPath = localPathFromUrl(currentUrl);
        wallpaperPaths = std::move(paths);
        const int idx = wallpaperPaths.indexOf(currentPath);
        if (idx >= 0)
            wallpaperIndex = idx;
        else if (wallpaperIndex >= wallpaperPaths.size())
            wallpaperIndex = 0;
    }
};

BackgroundManager::BackgroundManager(SettingsStore &settings)
    : d(std::make_unique<Impl>(Impl{settings}))
{
    const QString userPath = settings.getString(QStringLiteral("backgroundUserPath"));
    qCInfo(lcBg) << "init source" << backgroundSource()
                 << "userFolder" << (userPath.isEmpty() ? QStringLiteral("(none)") : userPath);
    d->reloadPaths();
    if (!d->wallpaperPaths.isEmpty())
        d->currentUrl = toDisplayUrl(d->wallpaperPaths.first());
    qCInfo(lcBg) << "folder files" << d->wallpaperPaths.size() << "current" << currentImageUrl();
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
    qCInfo(lcBg) << "setSource" << backgroundSource() << "→" << source
                 << "art" << d->spotifyArtUrl << "wallpaper" << d->currentUrl;
    d->settings.setString(QStringLiteral("backgroundSource"), source);
}

bool BackgroundManager::setSpotifyArtUrl(const QString &url)
{
    if (d->spotifyArtUrl == url)
        return false;
    qCInfo(lcBg) << "spotify art" << (url.isEmpty() ? QStringLiteral("(cleared)") : url)
                 << "source" << backgroundSource();
    d->spotifyArtUrl = url;
    return true;
}

void BackgroundManager::setUserWallpaperFolder(const QString &path)
{
    qCInfo(lcBg) << "setUserFolder" << path;
    d->settings.setString(QStringLiteral("backgroundUserPath"), path);
    d->wallpaperIndex = 0;
    d->lastRotationMs = 0;
    d->reloadPaths();
    if (d->wallpaperPaths.isEmpty()) {
        d->currentUrl.clear();
        return;
    }
    d->currentUrl = toDisplayUrl(d->wallpaperPaths.first());
    qCInfo(lcBg) << "folder loaded" << d->wallpaperPaths.size() << "first" << d->currentUrl;
}

bool BackgroundManager::tick()
{
    // Album art pins the background. Empty art (or wallpaper source) still
    // rotates the folder — otherwise a stuck first file sits behind the UI.
    if (backgroundSource() == QStringLiteral("spotify") && !d->spotifyArtUrl.isEmpty())
        return false;
    if (d->wallpaperPaths.isEmpty() && d->settings.getString(QStringLiteral("backgroundUserPath")).isEmpty())
        return false;

    d->reloadPaths();
    if (d->wallpaperPaths.isEmpty())
        return false;

    const QString currentPath = localPathFromUrl(d->currentUrl);
    const QString livePath = currentPath.isEmpty()
        ? d->wallpaperPaths.at(d->wallpaperIndex)
        : currentPath;
    const QString liveUrl = toDisplayUrl(livePath);
    if (liveUrl != d->currentUrl) {
        d->previousUrl = d->currentUrl;
        d->currentUrl = liveUrl;
        qCInfo(lcBg) << "file rewritten" << d->currentUrl;
        return true;
    }

    const int rotationSec = d->settings.getInt(QStringLiteral("backgroundRotationSec"), 300);
    d->lastRotationMs += 1000;
    if (d->lastRotationMs < rotationSec * 1000LL)
        return false;
    d->lastRotationMs = 0;
    if (d->wallpaperPaths.size() < 2)
        return false;

    d->previousUrl = d->currentUrl;
    d->wallpaperIndex = (d->wallpaperIndex + 1) % d->wallpaperPaths.size();
    const QString &next = d->wallpaperPaths.at(d->wallpaperIndex);
    d->currentUrl = toDisplayUrl(next);
    qCInfo(lcBg) << "rotate" << d->wallpaperIndex << "/" << d->wallpaperPaths.size()
                 << d->previousUrl << "→" << d->currentUrl;
    return true;
}

} // namespace polomodoro
