#pragma once

#include <QObject>
#include <memory>

class QWebEnginePage;

namespace polomodoro {

class SpotifyArtBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString artUrl READ artUrl NOTIFY metadataChanged)
    Q_PROPERTY(QString trackTitle READ trackTitle NOTIFY metadataChanged)
    Q_PROPERTY(QString artistName READ artistName NOTIFY metadataChanged)
    Q_PROPERTY(QString nowPlayingLabel READ nowPlayingLabel NOTIFY metadataChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY metadataChanged)
public:
    explicit SpotifyArtBridge(QObject *parent = nullptr);
    ~SpotifyArtBridge();

    Q_INVOKABLE void attachWebPage(QWebEnginePage *page);
    Q_INVOKABLE void startPolling(int intervalMs = 2000);

    QString artUrl() const;
    QString trackTitle() const;
    QString artistName() const;
    QString nowPlayingLabel() const;
    bool isPlaying() const;

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();

signals:
    void metadataChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
