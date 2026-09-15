#pragma once

#include <QObject>

namespace polomodoro {

class SpotifyArtBridge;

class SpotifyController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playbackChanged)
    Q_PROPERTY(QString nowPlayingLabel READ nowPlayingLabel NOTIFY playbackChanged)
    Q_PROPERTY(bool premiumRequired READ premiumRequired NOTIFY playbackChanged)
    Q_PROPERTY(SpotifyArtBridge *bridge READ bridge CONSTANT)
public:
    explicit SpotifyController(SpotifyArtBridge &bridge, QObject *parent = nullptr);

    bool ready() const;
    bool isPlaying() const;
    QString nowPlayingLabel() const;
    bool premiumRequired() const;
    SpotifyArtBridge *bridge() const;

    Q_INVOKABLE void attach(QObject *webView);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();

signals:
    void readyChanged();
    void playbackChanged();

private:
    SpotifyArtBridge &m_bridge;
    bool m_ready = false;
    bool m_premiumRequired = false;
};

} // namespace polomodoro
