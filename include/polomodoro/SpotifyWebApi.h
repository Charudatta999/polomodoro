#pragma once

#include <QObject>
#include <QVariantList>
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;

namespace polomodoro {

class SettingsStore;

// Browsing/search/devices/playlists via the Spotify Web API. Playback
// transport itself is MprisController's job for a local target; this class
// only issues the Web API call to *move* playback to a device
// (transferTo/play), never polls for now-playing state.
//
// Auth is OAuth PKCE — no client secret ships, since PKCE's whole point is
// that the public client id is safe to embed. What is NOT safe to embed is
// the refresh token, which must live in the system keyring, never the
// settings table (see rulebook: the SQLite file is meant to be copyable as a
// backup). Without qtkeychain installed, the token is kept in memory only for
// the running session; authState reports "none" again on every restart, and
// this is surfaced to the user rather than silently degraded.
class SpotifyWebApi : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString authState READ authState NOTIFY authStateChanged) // none|linking|linked|expired
    Q_PROPERTY(bool offline READ offline NOTIFY offlineChanged)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QString currentDeviceId READ currentDeviceId NOTIFY devicesChanged)
    Q_PROPERTY(QString currentDeviceName READ currentDeviceName NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList playlists READ playlists NOTIFY playlistsChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(bool clientConfigured READ clientConfigured NOTIFY authStateChanged)
public:
    explicit SpotifyWebApi(SettingsStore &settings, QObject *parent = nullptr);
    ~SpotifyWebApi() override;

    QString authState() const;
    bool offline() const;
    QVariantList devices() const;
    QString currentDeviceId() const;
    QString currentDeviceName() const;
    QVariantList playlists() const;
    QVariantList results() const;
    // False until a client id is set (Settings > Spotify). PKCE needs one
    // registered at developer.spotify.com; there is no default that would
    // work for every install, so none is invented here.
    bool clientConfigured() const;

    Q_INVOKABLE void beginPkce();
    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void play(const QString &uri);
    Q_INVOKABLE void transferTo(const QString &deviceId);
    Q_INVOKABLE void signOut();

signals:
    void authStateChanged();
    void offlineChanged();
    void devicesChanged();
    void playlistsChanged();
    void resultsChanged();
    // Carries the callback URL's query string once the loopback server
    // catches the redirect, for the code-exchange step.
    void pkceRedirectReceived(const QString &query);

private:
    struct Impl;
    std::unique_ptr<Impl> d;

    void exchangeCodeForToken(const QString &code);
    void refreshAccessToken();
    void fetchDevices();
    void fetchPlaylists();
    void checkConnectivity();
};

} // namespace polomodoro
