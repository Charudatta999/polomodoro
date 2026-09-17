#pragma once

#include <memory>
#include <QString>
#include <QStringList>

namespace polomodoro {

class SettingsStore;

class BackgroundManager {
public:
    explicit BackgroundManager(SettingsStore &settings);
    ~BackgroundManager();

    QString currentImageUrl() const;
    QString previousImageUrl() const;
    QString backgroundSource() const;

    void setBackgroundSource(const QString &source);
    // Returns true when the art URL actually changed.
    bool setSpotifyArtUrl(const QString &url);
    void setUserWallpaperFolder(const QString &path);
    // Returns true when the wallpaper URL actually changed.
    bool tick();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
