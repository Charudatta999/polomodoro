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
    void setSpotifyArtUrl(const QString &url);
    void tick();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
