#include "polomodoro/SpotifyWebApi.h"
#include "polomodoro/SettingsStore.h"

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QSet>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#ifdef POLOMODORO_HAVE_KEYCHAIN
#include <qt6keychain/keychain.h>
#endif

namespace polomodoro {

Q_LOGGING_CATEGORY(lcSpotify, "polomodoro.spotify")

namespace {

constexpr const char *kAuthorizeUrl = "https://accounts.spotify.com/authorize";
constexpr const char *kTokenUrl = "https://accounts.spotify.com/api/token";
constexpr const char *kApiBase = "https://api.spotify.com/v1";
constexpr const char *kScopes =
    "user-read-playback-state user-modify-playback-state user-read-currently-playing "
    "playlist-read-private playlist-read-collaborative streaming";
constexpr const char *kKeychainService = "Polomodoro";
constexpr const char *kKeychainKey = "spotify_refresh_token";

QString base64UrlNoPad(const QByteArray &bytes)
{
    return QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

// RFC 7636: 43-128 chars from [A-Za-z0-9-._~]. Base64url of random bytes
// already draws from a subset of that alphabet, so this is a valid verifier
// without needing a custom charset generator.
QString makeCodeVerifier()
{
    QByteArray raw(64, Qt::Uninitialized);
    for (int i = 0; i < raw.size(); ++i)
        raw[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    return base64UrlNoPad(raw);
}

QString codeChallengeFor(const QString &verifier)
{
    const QByteArray hash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
    return base64UrlNoPad(hash);
}

QString makeState()
{
    return base64UrlNoPad([] {
        QByteArray raw(16, Qt::Uninitialized);
        for (int i = 0; i < raw.size(); ++i)
            raw[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
        return raw;
    }());
}

} // namespace

struct SpotifyWebApi::Impl {
    SettingsStore &settings;
    QNetworkAccessManager net;

    QString authState = QStringLiteral("none");
    bool offline = false;

    QString accessToken;
    QString refreshTokenMemory; // used only when qtkeychain is unavailable
    QTimer refreshTimer;

    // In-flight PKCE attempt state.
    QString pendingVerifier;
    QString pendingState;
    std::unique_ptr<QTcpServer> pkceServer;
    quint16 redirectPort = 8888;

    QVariantList devices;
    QString currentDeviceId;
    QString currentDeviceName;
    QString preferredDeviceId; // user pick via transferTo; empty = prefer local spotifyd
    bool preferredIsUserPick = false;
    QSet<QString> rejectedDeviceIds;
    QVariantList playlists;
    QVariantList results;
    QString pendingPlayUri; // set when play() had no device yet and is waiting on a fetchDevices() round-trip
    int playRetryCount = 0;
    QTimer devicePollTimer;
    QTimer playbackTimer;
    QString lastDeviceFingerprint;
    bool remotePlaying = false;
    QString remoteTitle;
    QString remoteArtist;
    QString remoteArtUrl;
    double remotePositionRatio = 0;
    QString remotePositionLabel = QStringLiteral("0:00");
    QString remoteDurationLabel = QStringLiteral("0:00");

    explicit Impl(SettingsStore &s) : settings(s) {}
};

SpotifyWebApi::SpotifyWebApi(SettingsStore &settings, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>(settings))
{
    d->redirectPort = static_cast<quint16>(settings.getInt(QStringLiteral("spotifyRedirectPort"), 8888));

    connect(&d->refreshTimer, &QTimer::timeout, this, &SpotifyWebApi::refreshAccessToken);
    d->devicePollTimer.setInterval(8000);
    connect(&d->devicePollTimer, &QTimer::timeout, this, &SpotifyWebApi::fetchDevices);
    d->playbackTimer.setInterval(1000);
    connect(&d->playbackTimer, &QTimer::timeout, this, &SpotifyWebApi::fetchPlayback);

    checkConnectivity();
    qCInfo(lcSpotify) << "constructed; clientConfigured=" << clientConfigured()
                      << "redirectPort=" << d->redirectPort;

#ifdef POLOMODORO_HAVE_KEYCHAIN
    // Attempt a silent sign-in from a token left in the keyring by a previous
    // run. Nothing surfaces to the user until this resolves either way.
    auto *readJob = new QKeychain::ReadPasswordJob(QString::fromLatin1(kKeychainService));
    readJob->setKey(QString::fromLatin1(kKeychainKey));
    connect(readJob, &QKeychain::Job::finished, this, [this](QKeychain::Job *job) {
        auto *read = static_cast<QKeychain::ReadPasswordJob *>(job);
        if (read->error() == QKeychain::NoError && !read->textData().isEmpty()) {
            qCInfo(lcSpotify) << "refresh token loaded from keyring";
            d->refreshTokenMemory = read->textData();
            refreshAccessToken();
        } else {
            qCInfo(lcSpotify) << "no refresh token in keyring (error" << int(read->error()) << ")";
        }
        job->deleteLater();
    });
    readJob->start();
#endif
}

SpotifyWebApi::~SpotifyWebApi() = default;

QString SpotifyWebApi::authState() const { return d->authState; }
bool SpotifyWebApi::offline() const { return d->offline; }
QVariantList SpotifyWebApi::devices() const { return d->devices; }
QString SpotifyWebApi::currentDeviceId() const { return d->currentDeviceId; }
QString SpotifyWebApi::currentDeviceName() const { return d->currentDeviceName; }
QVariantList SpotifyWebApi::playlists() const { return d->playlists; }
QVariantList SpotifyWebApi::results() const { return d->results; }

bool SpotifyWebApi::controllingRemote() const
{
    if (d->currentDeviceId.isEmpty())
        return false;
    for (const QVariant &v : d->devices) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toString() == d->currentDeviceId)
            return !m.value(QStringLiteral("isLocal")).toBool();
    }
    return true;
}

bool SpotifyWebApi::remotePlaying() const { return d->remotePlaying; }
QString SpotifyWebApi::remoteTitle() const { return d->remoteTitle; }
QString SpotifyWebApi::remoteArtist() const { return d->remoteArtist; }
QString SpotifyWebApi::remoteArtUrl() const { return d->remoteArtUrl; }
double SpotifyWebApi::remotePositionRatio() const { return d->remotePositionRatio; }
QString SpotifyWebApi::remotePositionLabel() const { return d->remotePositionLabel; }
QString SpotifyWebApi::remoteDurationLabel() const { return d->remoteDurationLabel; }

bool SpotifyWebApi::clientConfigured() const
{
    return !d->settings.getString(QStringLiteral("spotifyClientId")).isEmpty();
}

void SpotifyWebApi::checkConnectivity()
{
    // A lightweight, dependency-free reachability probe: try the token
    // endpoint's host. QNetworkInformation would be the "correct" API but
    // pulls in a platform backend that may not be loaded in every
    // environment; a HEAD-style GET with a short timeout is simpler and
    // degrades safely (offline stays false only once this confirms up).
    auto *reply = d->net.get(QNetworkRequest(QUrl(QStringLiteral("https://accounts.spotify.com/"))));
    QTimer::singleShot(4000, reply, [reply]() { if (reply->isRunning()) reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const bool reachable = reply->error() == QNetworkReply::NoError
            || reply->error() == QNetworkReply::ContentNotFoundError; // any HTTP response = reachable
        if (d->offline == reachable) {
            d->offline = !reachable;
            emit offlineChanged();
        }
        reply->deleteLater();
    });
}

void SpotifyWebApi::beginPkce()
{
    if (!clientConfigured()) {
        qCWarning(lcSpotify) << "beginPkce: no client id configured";
        return;
    }

    d->pendingVerifier = makeCodeVerifier();
    d->pendingState = makeState();

    d->pkceServer = std::make_unique<QTcpServer>();
    if (!d->pkceServer->listen(QHostAddress::LocalHost, d->redirectPort)) {
        qCWarning(lcSpotify) << "PKCE loopback listen failed on port" << d->redirectPort
                             << d->pkceServer->errorString();
        d->pkceServer.reset();
        return;
    }
    qCInfo(lcSpotify) << "PKCE loopback listening on 127.0.0.1:" << d->redirectPort;

    connect(d->pkceServer.get(), &QTcpServer::newConnection, this, [this]() {
        QTcpSocket *socket = d->pkceServer->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            // The request line can arrive split across reads; wait for a
            // full line rather than parsing a partial one.
            if (!socket->canReadLine())
                return;
            const QByteArray requestLine = socket->readLine();
            // "GET /callback?code=...&state=... HTTP/1.1"
            const QList<QByteArray> parts = requestLine.split(' ');
            if (parts.size() >= 2) {
                const QUrl url(QStringLiteral("http://127.0.0.1") + QString::fromLatin1(parts.at(1)));
                const QUrlQuery query(url);
                const QString state = query.queryItemValue(QStringLiteral("state"));
                const QString code = query.queryItemValue(QStringLiteral("code"));
                const QString error = query.queryItemValue(QStringLiteral("error"));

                static const char *kResponseOk =
                    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                    "<html><body style='font-family:sans-serif'>Signed in — you can close this tab.</body></html>";
                static const char *kResponseErr =
                    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                    "<html><body style='font-family:sans-serif'>Sign-in failed or was cancelled — you can close this tab.</body></html>";

                socket->write(error.isEmpty() && state == d->pendingState ? kResponseOk : kResponseErr);
                socket->flush();
                socket->disconnectFromHost();

                if (error.isEmpty() && state == d->pendingState && !code.isEmpty()) {
                    qCInfo(lcSpotify) << "PKCE redirect ok — exchanging code";
                    exchangeCodeForToken(code);
                } else {
                    qCWarning(lcSpotify) << "PKCE redirect rejected error=" << error
                                         << "stateMatch=" << (state == d->pendingState);
                }
            }
            // socket is a child of pkceServer: resetting the server here,
            // synchronously, would destroy socket while this very handler is
            // still executing on its call stack (readyRead is one of
            // socket's own signals) — undefined behavior, crashes shortly
            // after. Detach and defer both deletions to the next event loop
            // turn instead.
            socket->setParent(nullptr);
            socket->deleteLater();
            QTimer::singleShot(0, this, [this]() { d->pkceServer.reset(); }); // one redirect is all we need
        });
    });

    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"), d->settings.getString(QStringLiteral("spotifyClientId")));
    q.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    q.addQueryItem(QStringLiteral("redirect_uri"),
                   QStringLiteral("http://127.0.0.1:%1/callback").arg(d->redirectPort));
    q.addQueryItem(QStringLiteral("code_challenge_method"), QStringLiteral("S256"));
    q.addQueryItem(QStringLiteral("code_challenge"), codeChallengeFor(d->pendingVerifier));
    q.addQueryItem(QStringLiteral("state"), d->pendingState);
    q.addQueryItem(QStringLiteral("scope"), QString::fromLatin1(kScopes));

    QUrl authorize(QString::fromLatin1(kAuthorizeUrl));
    authorize.setQuery(q);
    QDesktopServices::openUrl(authorize);

    d->authState = QStringLiteral("linking");
    emit authStateChanged();
    qCInfo(lcSpotify) << "opened Spotify authorize URL";
}

void SpotifyWebApi::exchangeCodeForToken(const QString &code)
{
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    body.addQueryItem(QStringLiteral("code"), code);
    body.addQueryItem(QStringLiteral("redirect_uri"),
                       QStringLiteral("http://127.0.0.1:%1/callback").arg(d->redirectPort));
    body.addQueryItem(QStringLiteral("client_id"), d->settings.getString(QStringLiteral("spotifyClientId")));
    body.addQueryItem(QStringLiteral("code_verifier"), d->pendingVerifier);

    QNetworkRequest req{QUrl(QString::fromLatin1(kTokenUrl))};
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/x-www-form-urlencoded"));

    QNetworkReply *reply = d->net.post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(lcSpotify) << "token exchange failed:" << reply->errorString()
                                 << reply->readAll();
            d->authState = QStringLiteral("none");
            emit authStateChanged();
            return;
        }
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        d->accessToken = obj.value(QStringLiteral("access_token")).toString();
        const QString refreshToken = obj.value(QStringLiteral("refresh_token")).toString();
        const int expiresIn = obj.value(QStringLiteral("expires_in")).toInt(3600);

        if (!refreshToken.isEmpty()) {
#ifdef POLOMODORO_HAVE_KEYCHAIN
            auto *writeJob = new QKeychain::WritePasswordJob(QString::fromLatin1(kKeychainService));
            writeJob->setKey(QString::fromLatin1(kKeychainKey));
            writeJob->setTextData(refreshToken);
            connect(writeJob, &QKeychain::Job::finished, writeJob, &QObject::deleteLater);
            writeJob->start();
#else
            // No keyring available: kept in memory for this run only, so
            // signing in again is required after every restart. Told to the
            // user via Settings rather than silently losing the session.
            d->refreshTokenMemory = refreshToken;
#endif
        }

        d->authState = QStringLiteral("linked");
        emit authStateChanged();
        qCInfo(lcSpotify) << "signed in; token expires in" << expiresIn << "s";

        d->refreshTimer.start(qMax(30, expiresIn - 60) * 1000);
        d->devicePollTimer.start();
        d->playbackTimer.start();
        fetchDevices();
        fetchPlaylists();
    });
}

void SpotifyWebApi::refreshAccessToken()
{
    const QString refreshToken =
#ifdef POLOMODORO_HAVE_KEYCHAIN
        d->refreshTokenMemory; // populated by the keyring read in the constructor
#else
        d->refreshTokenMemory;
#endif
    if (refreshToken.isEmpty() || !clientConfigured()) {
        qCInfo(lcSpotify) << "refreshAccessToken skipped; haveToken=" << !refreshToken.isEmpty()
                          << "clientConfigured=" << clientConfigured();
        return;
    }
    qCInfo(lcSpotify) << "refreshing access token";

    QUrlQuery body;
    body.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    body.addQueryItem(QStringLiteral("refresh_token"), refreshToken);
    body.addQueryItem(QStringLiteral("client_id"), d->settings.getString(QStringLiteral("spotifyClientId")));

    QNetworkRequest req{QUrl(QString::fromLatin1(kTokenUrl))};
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/x-www-form-urlencoded"));

    QNetworkReply *reply = d->net.post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(lcSpotify) << "token refresh failed:" << reply->errorString()
                                 << reply->readAll();
            d->authState = QStringLiteral("expired");
            emit authStateChanged();
            d->devicePollTimer.stop();
            d->playbackTimer.stop();
            return;
        }
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        d->accessToken = obj.value(QStringLiteral("access_token")).toString();
        const int expiresIn = obj.value(QStringLiteral("expires_in")).toInt(3600);
        // Spotify may rotate the refresh token on refresh; persist it again if so.
        const QString newRefresh = obj.value(QStringLiteral("refresh_token")).toString();
        if (!newRefresh.isEmpty()) {
            d->refreshTokenMemory = newRefresh;
#ifdef POLOMODORO_HAVE_KEYCHAIN
            auto *writeJob = new QKeychain::WritePasswordJob(QString::fromLatin1(kKeychainService));
            writeJob->setKey(QString::fromLatin1(kKeychainKey));
            writeJob->setTextData(newRefresh);
            connect(writeJob, &QKeychain::Job::finished, writeJob, &QObject::deleteLater);
            writeJob->start();
#endif
        }
        d->authState = QStringLiteral("linked");
        emit authStateChanged();
        qCInfo(lcSpotify) << "access token refreshed; expires in" << expiresIn << "s";
        d->refreshTimer.start(qMax(30, expiresIn - 60) * 1000);
        d->devicePollTimer.start();
        d->playbackTimer.start();
        fetchDevices();
        fetchPlaylists();
    });
}

void SpotifyWebApi::signOut()
{
    qCInfo(lcSpotify) << "signOut";
    d->accessToken.clear();
    d->refreshTokenMemory.clear();
    d->refreshTimer.stop();
    d->devicePollTimer.stop();
    d->playbackTimer.stop();
    d->devices.clear();
    d->playlists.clear();
    d->results.clear();
    d->currentDeviceId.clear();
    d->currentDeviceName.clear();
    d->preferredDeviceId.clear();
    d->preferredIsUserPick = false;
    d->rejectedDeviceIds.clear();
    d->authState = QStringLiteral("none");

#ifdef POLOMODORO_HAVE_KEYCHAIN
    auto *deleteJob = new QKeychain::DeletePasswordJob(QString::fromLatin1(kKeychainService));
    deleteJob->setKey(QString::fromLatin1(kKeychainKey));
    connect(deleteJob, &QKeychain::Job::finished, deleteJob, &QObject::deleteLater);
    deleteJob->start();
#endif

    emit authStateChanged();
    emit devicesChanged();
    emit playlistsChanged();
    emit resultsChanged();
}

namespace {
QNetworkRequest authedRequest(const QString &path, const QString &accessToken)
{
    QNetworkRequest req{QUrl(QString::fromLatin1(kApiBase) + path)};
    req.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    return req;
}
} // namespace

void SpotifyWebApi::refreshDevices()
{
    fetchDevices();
}

QString SpotifyWebApi::configuredDeviceName() const
{
    const QString n = d->settings.getString(QStringLiteral("spotifyDeviceName")).trimmed();
    return n.isEmpty() ? QStringLiteral("Polomodoro") : n;
}

QString SpotifyWebApi::deviceNameForId(const QString &id) const
{
    for (const QVariant &v : d->devices) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toString() == id)
            return m.value(QStringLiteral("name")).toString();
    }
    return {};
}

QString SpotifyWebApi::resolveTargetDeviceId() const
{
    auto usable = [this](const QVariantMap &m) -> QString {
        const QString id = m.value(QStringLiteral("id")).toString();
        if (id.isEmpty() || d->rejectedDeviceIds.contains(id))
            return {};
        if (m.value(QStringLiteral("isRestricted")).toBool())
            return {};
        return id;
    };

    QString localId;
    QString activeId;
    QString firstId;
    for (const QVariant &v : d->devices) {
        const QVariantMap m = v.toMap();
        const QString id = usable(m);
        if (id.isEmpty())
            continue;
        if (firstId.isEmpty())
            firstId = id;
        if (m.value(QStringLiteral("isLocal")).toBool() && localId.isEmpty())
            localId = id;
        if (m.value(QStringLiteral("isActive")).toBool() && activeId.isEmpty())
            activeId = id;
    }

    if (d->preferredIsUserPick && !d->preferredDeviceId.isEmpty()) {
        for (const QVariant &v : d->devices) {
            const QString id = usable(v.toMap());
            if (id == d->preferredDeviceId)
                return id;
        }
        qCWarning(lcSpotify) << "user-picked device gone or unusable:" << d->preferredDeviceId;
    }

    // Playback belongs on this machine's spotifyd. Never fall through to a
    // stale phone/web player just because Spotify still marks it active —
    // that is the 404 "Not found" loop from PUT /me/player/play?device_id=.
    if (!localId.isEmpty())
        return localId;
    // One Connect device and it is this machine: treat it as local even if
    // the name setting was empty (SpotifydManager still advertises "Polomodoro").
    if (!firstId.isEmpty() && d->devices.size() == 1)
        return firstId;
    if (d->preferredIsUserPick)
        return activeId.isEmpty() ? firstId : activeId;
    return {};
}

void SpotifyWebApi::fetchDevices()
{
    if (d->accessToken.isEmpty())
        return;
    QNetworkReply *reply = d->net.get(authedRequest(QStringLiteral("/me/player/devices"), d->accessToken));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray body = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(lcSpotify) << "fetchDevices failed HTTP" << status << reply->errorString() << body.left(400);
            return;
        }
        const QJsonArray arr = QJsonDocument::fromJson(body)
                                    .object().value(QStringLiteral("devices")).toArray();
        const QString localName = configuredDeviceName();

        QVariantList out;
        for (const QJsonValue &v : arr) {
            const QJsonObject o = v.toObject();
            QVariantMap m;
            const QString id = o.value(QStringLiteral("id")).toString();
            const QString name = o.value(QStringLiteral("name")).toString().trimmed();
            const bool restricted = o.value(QStringLiteral("is_restricted")).toBool();
            const bool active = o.value(QStringLiteral("is_active")).toBool();
            const QString type = o.value(QStringLiteral("type")).toString();
            const bool isLocal = !localName.isEmpty()
                && (name.compare(localName, Qt::CaseInsensitive) == 0
                    || name.startsWith(localName + QLatin1Char(' '), Qt::CaseInsensitive));
            m[QStringLiteral("id")] = id;
            m[QStringLiteral("name")] = name;
            m[QStringLiteral("isLocal")] = isLocal;
            m[QStringLiteral("isRestricted")] = restricted;
            m[QStringLiteral("isActive")] = active;
            m[QStringLiteral("type")] = type;
            out.push_back(m);
        }
        QString fingerprint;
        for (const QVariant &v : out) {
            const QVariantMap m = v.toMap();
            fingerprint += m.value(QStringLiteral("id")).toString() + QLatin1Char(':')
                + m.value(QStringLiteral("isActive")).toString() + QLatin1Char(';');
        }
        const bool changed = fingerprint != d->lastDeviceFingerprint;
        d->lastDeviceFingerprint = fingerprint;
        if (changed) {
            for (const QVariant &v : out) {
                const QVariantMap m = v.toMap();
                qCInfo(lcSpotify) << "device" << m.value(QStringLiteral("name")).toString()
                                  << "id" << m.value(QStringLiteral("id")).toString()
                                  << "type" << m.value(QStringLiteral("type")).toString()
                                  << "active" << m.value(QStringLiteral("isActive")).toBool()
                                  << "restricted" << m.value(QStringLiteral("isRestricted")).toBool()
                                  << "local" << m.value(QStringLiteral("isLocal")).toBool();
            }
        }

        d->devices = out;

        const QString chosen = resolveTargetDeviceId();
        d->currentDeviceId = chosen;
        d->currentDeviceName = deviceNameForId(chosen);
        if (changed)
            qCInfo(lcSpotify) << "devices" << out.size() << "target" << d->currentDeviceName
                              << d->currentDeviceId;
        emit devicesChanged();

        if (!d->pendingPlayUri.isEmpty()) {
            const QString uri = d->pendingPlayUri;
            d->pendingPlayUri.clear();
            play(uri);
        }
    });
}

void SpotifyWebApi::fetchPlaylists()
{
    if (d->accessToken.isEmpty())
        return;
    QNetworkReply *reply =
        d->net.get(authedRequest(QStringLiteral("/me/playlists?limit=50"), d->accessToken));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(lcSpotify) << "fetchPlaylists failed:" << reply->errorString() << reply->readAll().left(400);
            return;
        }
        const QJsonArray arr = QJsonDocument::fromJson(reply->readAll())
                                    .object().value(QStringLiteral("items")).toArray();
        QVariantList out;
        for (const QJsonValue &v : arr) {
            const QJsonObject o = v.toObject();
            QVariantMap m;
            m[QStringLiteral("name")] = o.value(QStringLiteral("name")).toString();
            // The playlist's track-count ref object is documented as
            // "tracks": {"total": N}, but this account's live responses
            // return it under the key "items" instead — check both.
            const QJsonObject trackRef = o.contains(QStringLiteral("tracks"))
                ? o.value(QStringLiteral("tracks")).toObject()
                : o.value(QStringLiteral("items")).toObject();
            m[QStringLiteral("subtitle")] =
                QStringLiteral("%1 tracks").arg(trackRef.value(QStringLiteral("total")).toInt());
            const QJsonArray images = o.value(QStringLiteral("images")).toArray();
            m[QStringLiteral("artUrl")] =
                images.isEmpty() ? QString() : images.first().toObject().value(QStringLiteral("url")).toString();
            m[QStringLiteral("uri")] = o.value(QStringLiteral("uri")).toString();
            out.push_back(m);
        }
        d->playlists = out;
        qCInfo(lcSpotify) << "playlists" << out.size();
        emit playlistsChanged();
    });
}

void SpotifyWebApi::search(const QString &query)
{
    if (d->accessToken.isEmpty() || query.trimmed().isEmpty())
        return;
    qCInfo(lcSpotify) << "search" << query;
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("q"), query);
    q.addQueryItem(QStringLiteral("type"), QStringLiteral("track,playlist"));
    // No explicit limit: apps in Spotify's default "Development mode" quota
    // (i.e. not granted Extended Quota Mode) get a 400 "Invalid limit" for
    // values the public docs claim are valid (confirmed live: limit=20
    // rejected, omitting it entirely succeeds and Spotify applies its own
    // enforced default — 5, for this app). Forcing a number here would just
    // be guessing at a cap Spotify doesn't document per quota tier.

    QNetworkRequest req = authedRequest(QStringLiteral("/search"), d->accessToken);
    QUrl url = req.url();
    url.setQuery(q);
    req.setUrl(url);

    QNetworkReply *reply = d->net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(lcSpotify) << "search failed:" << reply->errorString() << reply->readAll().left(400);
            return;
        }
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        QVariantList out;

        const QJsonArray tracks = obj.value(QStringLiteral("tracks")).toObject()
                                       .value(QStringLiteral("items")).toArray();
        for (const QJsonValue &v : tracks) {
            const QJsonObject o = v.toObject();
            QVariantMap m;
            m[QStringLiteral("name")] = o.value(QStringLiteral("name")).toString();
            QStringList artists;
            for (const QJsonValue &a : o.value(QStringLiteral("artists")).toArray())
                artists << a.toObject().value(QStringLiteral("name")).toString();
            m[QStringLiteral("subtitle")] = artists.join(QStringLiteral(", "));
            const QJsonArray images = o.value(QStringLiteral("album")).toObject()
                                           .value(QStringLiteral("images")).toArray();
            m[QStringLiteral("artUrl")] =
                images.isEmpty() ? QString() : images.last().toObject().value(QStringLiteral("url")).toString();
            m[QStringLiteral("uri")] = o.value(QStringLiteral("uri")).toString();
            out.push_back(m);
        }

        const QJsonArray lists = obj.value(QStringLiteral("playlists")).toObject()
                                      .value(QStringLiteral("items")).toArray();
        for (const QJsonValue &v : lists) {
            const QJsonObject o = v.toObject();
            QVariantMap m;
            m[QStringLiteral("name")] = o.value(QStringLiteral("name")).toString();
            m[QStringLiteral("subtitle")] = QStringLiteral("Playlist");
            const QJsonArray images = o.value(QStringLiteral("images")).toArray();
            m[QStringLiteral("artUrl")] =
                images.isEmpty() ? QString() : images.first().toObject().value(QStringLiteral("url")).toString();
            m[QStringLiteral("uri")] = o.value(QStringLiteral("uri")).toString();
            out.push_back(m);
        }

        d->results = out;
        qCInfo(lcSpotify) << "search results" << out.size();
        emit resultsChanged();
    });
}

void SpotifyWebApi::play(const QString &uri)
{
    if (d->accessToken.isEmpty()) {
        qCWarning(lcSpotify) << "play: not signed in (Web API)";
        return;
    }
    if (uri.trimmed().isEmpty())
        return;

    qCInfo(lcSpotify) << "play" << uri << "retry" << d->playRetryCount
                      << "knownDevices" << d->devices.size();

    const QString deviceId = resolveTargetDeviceId();
    if (deviceId.isEmpty()) {
        if (d->playRetryCount < 2) {
            d->pendingPlayUri = uri;
            ++d->playRetryCount;
            qCInfo(lcSpotify) << "play: Polomodoro is not in the Connect device list yet — refetching";
            fetchDevices();
            return;
        }
        d->playRetryCount = 0;
        qCWarning(lcSpotify) << "play: refusing to target a non-local device. "
                                "spotifyd is not visible to Spotify yet. Sign in to spotifyd in "
                                "Settings → Music (librespot OAuth — not the playlist Sign in).";
        return;
    }

    d->playRetryCount = 0;
    putPlay(uri, deviceId);
}

void SpotifyWebApi::putPlay(const QString &uri, const QString &deviceId)
{
    QJsonObject body;
    if (uri.contains(QStringLiteral(":track:")) || uri.contains(QStringLiteral(":episode:")))
        body[QStringLiteral("uris")] = QJsonArray{uri};
    else
        body[QStringLiteral("context_uri")] = uri;

    qCInfo(lcSpotify) << "PUT /me/player/play device" << deviceNameForId(deviceId) << deviceId << uri;
    QNetworkRequest req = authedRequest(
        QStringLiteral("/me/player/play?device_id=") + deviceId, d->accessToken);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = d->net.put(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, uri, deviceId]() {
        reply->deleteLater();
        const QByteArray bodyBytes = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status >= 200 && status < 300) {
            qCInfo(lcSpotify) << "play ok HTTP" << status << "on" << deviceNameForId(deviceId);
            d->rejectedDeviceIds.remove(deviceId);
            d->playRetryCount = 0;
            QTimer::singleShot(500, this, &SpotifyWebApi::fetchDevices);
            return;
        }
        qCWarning(lcSpotify) << "play failed HTTP" << status << reply->errorString() << bodyBytes.left(500);
        if (status == 404) {
            bool wasLocal = false;
            for (const QVariant &v : d->devices) {
                const QVariantMap m = v.toMap();
                if (m.value(QStringLiteral("id")).toString() == deviceId)
                    wasLocal = m.value(QStringLiteral("isLocal")).toBool();
            }
            if (!wasLocal)
                d->rejectedDeviceIds.insert(deviceId);
            qCInfo(lcSpotify) << "play 404 on" << deviceId << "local=" << wasLocal
                              << "— refetching devices";
            if (d->preferredDeviceId == deviceId)
                d->preferredIsUserPick = false;
            d->pendingPlayUri = uri;
            ++d->playRetryCount;
            fetchDevices();
        }
    });
}

void SpotifyWebApi::transferTo(const QString &deviceId)
{
    if (d->accessToken.isEmpty() || deviceId.isEmpty())
        return;
    d->preferredDeviceId = deviceId;
    d->preferredIsUserPick = true;
    d->rejectedDeviceIds.remove(deviceId);
    qCInfo(lcSpotify) << "transferTo" << deviceNameForId(deviceId) << deviceId;

    QJsonObject body;
    body[QStringLiteral("device_ids")] = QJsonArray{deviceId};
    body[QStringLiteral("play")] = true;

    QNetworkRequest req = authedRequest(QStringLiteral("/me/player"), d->accessToken);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = d->net.put(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, deviceId]() {
        reply->deleteLater();
        const QByteArray bodyBytes = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status >= 200 && status < 300) {
            qCInfo(lcSpotify) << "transfer ok HTTP" << status;
            fetchDevices();
            return;
        }
        qCWarning(lcSpotify) << "transfer failed HTTP" << status << reply->errorString() << bodyBytes.left(400);
        if (status == 404)
            d->rejectedDeviceIds.insert(deviceId);
        fetchDevices();
    });
}

void SpotifyWebApi::playerCommand(const QString &method, const QString &path)
{
    if (d->accessToken.isEmpty())
        return;
    QNetworkRequest req = authedRequest(path, d->accessToken);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = method == QLatin1String("POST")
        ? d->net.post(req, QByteArray("{}"))
        : d->net.put(req, QByteArray("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, path]() {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qCInfo(lcSpotify) << "playerCommand" << path << "HTTP" << status;
        if (status >= 200 && status < 300)
            fetchPlayback();
        else
            qCWarning(lcSpotify) << "playerCommand failed" << reply->errorString() << reply->readAll().left(300);
    });
}

void SpotifyWebApi::pausePlayback() { playerCommand(QStringLiteral("PUT"), QStringLiteral("/me/player/pause")); }
void SpotifyWebApi::skipNext() { playerCommand(QStringLiteral("POST"), QStringLiteral("/me/player/next")); }
void SpotifyWebApi::skipPrevious() { playerCommand(QStringLiteral("POST"), QStringLiteral("/me/player/previous")); }
void SpotifyWebApi::togglePlayback()
{
    if (d->remotePlaying)
        pausePlayback();
    else
        playerCommand(QStringLiteral("PUT"), QStringLiteral("/me/player/play"));
}

void SpotifyWebApi::fetchPlayback()
{
    if (d->accessToken.isEmpty() || !controllingRemote())
        return;
    QNetworkReply *reply = d->net.get(authedRequest(QStringLiteral("/me/player"), d->accessToken));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return;
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        d->remotePlaying = obj.value(QStringLiteral("is_playing")).toBool();
        const QJsonObject item = obj.value(QStringLiteral("item")).toObject();
        d->remoteTitle = item.value(QStringLiteral("name")).toString();
        QStringList artists;
        for (const QJsonValue &a : item.value(QStringLiteral("artists")).toArray())
            artists << a.toObject().value(QStringLiteral("name")).toString();
        d->remoteArtist = artists.join(QStringLiteral(", "));
        const QJsonArray images = item.value(QStringLiteral("album")).toObject()
                                       .value(QStringLiteral("images")).toArray();
        d->remoteArtUrl = images.isEmpty() ? QString()
            : images.last().toObject().value(QStringLiteral("url")).toString();
        const qint64 durationMs = item.value(QStringLiteral("duration_ms")).toInt();
        const qint64 progressMs = obj.value(QStringLiteral("progress_ms")).toInt();
        d->remotePositionRatio = durationMs > 0 ? qBound(0.0, double(progressMs) / double(durationMs), 1.0) : 0.0;
        auto fmt = [](qint64 ms) {
            const qint64 s = qMax(0LL, ms) / 1000;
            return QStringLiteral("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
        };
        d->remotePositionLabel = fmt(progressMs);
        d->remoteDurationLabel = fmt(durationMs);
        emit remotePlaybackChanged();
    });
}

} // namespace polomodoro
