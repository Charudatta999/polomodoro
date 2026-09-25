#pragma once

#include <QObject>
#include <QVariantList>
#include <memory>

namespace polomodoro {

class SpotifyWebApi;

// Controls whatever MPRIS player is on the session bus — spotifyd, the
// official Spotify client, anything. When the Web API target is a remote
// Connect device, this class proxies transport to SpotifyWebApi so the
// now-playing strip does not branch.
class MprisController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playbackChanged)
    Q_PROPERTY(QString title READ title NOTIFY playbackChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY playbackChanged)
    Q_PROPERTY(QString album READ album NOTIFY playbackChanged)
    Q_PROPERTY(QString artUrl READ artUrl NOTIFY playbackChanged)
    Q_PROPERTY(QString nowPlayingLabel READ nowPlayingLabel NOTIFY playbackChanged)
    Q_PROPERTY(double positionRatio READ positionRatio NOTIFY positionChanged)
    Q_PROPERTY(QString positionLabel READ positionLabel NOTIFY positionChanged)
    Q_PROPERTY(QString durationLabel READ durationLabel NOTIFY positionChanged)
    Q_PROPERTY(QVariantList players READ players NOTIFY playersChanged)
    Q_PROPERTY(QString currentService READ currentService NOTIFY playersChanged)
    Q_PROPERTY(bool isSpotifyPlayer READ isSpotifyPlayer NOTIFY playbackChanged)
public:
    explicit MprisController(QObject *parent = nullptr);
    ~MprisController() override;

    void setSpotifyApi(SpotifyWebApi *api);

    bool available() const;
    bool isPlaying() const;
    QString title() const;
    QString artist() const;
    QString album() const;
    QString artUrl() const;
    QString nowPlayingLabel() const;
    double positionRatio() const;
    QString positionLabel() const;
    QString durationLabel() const;
    bool isSpotifyPlayer() const;
    QVariantList players() const;
    QString currentService() const;

    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    // ratio in [0,1] of the current track's length.
    Q_INVOKABLE void seekToRatio(double ratio);
    Q_INVOKABLE void selectPlayer(const QString &serviceName);

signals:
    void availableChanged();
    void playbackChanged();
    void positionChanged();
    void playersChanged();

private slots:
    // Targeted by QDBusConnection::connect's string-based SLOT() API in
    // attachToPlayer()/detachPlayer(), which requires a real moc-registered
    // slot — a plain private method is not resolvable through that API and
    // fails silently (a runtime "no such slot" warning, not a compile error).
    void refreshMetadata();
    void onNameOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);

private:
    struct Impl;
    std::unique_ptr<Impl> d;

    void attachToPlayer(const QString &serviceName);
    void detachPlayer();
    void refreshPosition();
    void findExistingPlayer();
    void rebuildPlayerList();
    QString identityFor(const QString &serviceName) const;
};

} // namespace polomodoro
