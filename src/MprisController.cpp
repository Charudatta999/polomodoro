#include "polomodoro/MprisController.h"
#include "polomodoro/SpotifyWebApi.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QLoggingCategory>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

namespace polomodoro {

Q_LOGGING_CATEGORY(lcMpris, "polomodoro.mpris")

namespace {

constexpr const char *kMprisPrefix = "org.mpris.MediaPlayer2.";
constexpr const char *kMprisPath = "/org/mpris/MediaPlayer2";
constexpr const char *kPlayerIface = "org.mpris.MediaPlayer2.Player";
constexpr const char *kPropsIface = "org.freedesktop.DBus.Properties";

QVariantMap demarshalMap(const QVariant &v)
{
    if (v.canConvert<QDBusArgument>())
        return qdbus_cast<QVariantMap>(v.value<QDBusArgument>());
    return v.toMap();
}

QString formatMs(qint64 ms)
{
    if (ms < 0)
        ms = 0;
    const qint64 totalSec = ms / 1000;
    return QStringLiteral("%1:%2")
        .arg(totalSec / 60)
        .arg(totalSec % 60, 2, 10, QLatin1Char('0'));
}

} // namespace

struct MprisController::Impl {
    QString serviceName;
    QDBusInterface *props = nullptr;
    QDBusInterface *player = nullptr;
    QTimer positionTimer;

    bool isPlaying = false;
    QString title;
    QString artist;
    QString album;
    QString artUrl;
    QString trackId;
    qint64 lengthUs = 0;

    double positionRatio = 0.0;
    QString positionLabel = QStringLiteral("0:00");
    QString durationLabel = QStringLiteral("0:00");
    SpotifyWebApi *api = nullptr;
    bool userPinned = false;
    QVariantList players;
};

MprisController::MprisController(QObject *parent) : QObject(parent), d(std::make_unique<Impl>())
{
    d->positionTimer.setInterval(1000);
    connect(&d->positionTimer, &QTimer::timeout, this, &MprisController::refreshPosition);

    auto *bus = QDBusConnection::sessionBus().interface();
    Q_UNUSED(bus);
    // Avoid QDBusConnectionInterface::serviceOwnerChanged — it is deprecated
    // and logs on every startup. NameOwnerChanged is the same signal.
    QDBusConnection::sessionBus().connect(
        QStringLiteral("org.freedesktop.DBus"),
        QStringLiteral("/org/freedesktop/DBus"),
        QStringLiteral("org.freedesktop.DBus"),
        QStringLiteral("NameOwnerChanged"),
        this, SLOT(onNameOwnerChanged(QString,QString,QString)));

    findExistingPlayer();
    rebuildPlayerList();
}

MprisController::~MprisController() = default;

void MprisController::setSpotifyApi(SpotifyWebApi *api)
{
    d->api = api;
    if (!api)
        return;
    connect(api, &SpotifyWebApi::remotePlaybackChanged, this, [this]() {
        emit playbackChanged();
        emit positionChanged();
        emit availableChanged();
    });
    connect(api, &SpotifyWebApi::devicesChanged, this, [this]() {
        emit availableChanged();
        emit playbackChanged();
    });
}

QString MprisController::identityFor(const QString &serviceName) const
{
    QDBusInterface props(serviceName, QString::fromLatin1(kMprisPath),
                         QString::fromLatin1(kPropsIface), QDBusConnection::sessionBus());
    QDBusReply<QVariant> reply = props.call(QStringLiteral("Get"),
                                            QStringLiteral("org.mpris.MediaPlayer2"),
                                            QStringLiteral("Identity"));
    if (reply.isValid()) {
        const QString id = reply.value().toString().trimmed();
        if (!id.isEmpty())
            return id;
    }
    QString tail = serviceName;
    tail.remove(QString::fromLatin1(kMprisPrefix));
    const int dot = tail.indexOf(QLatin1Char('.'));
    if (dot > 0)
        tail = tail.left(dot);
    return tail;
}

void MprisController::rebuildPlayerList()
{
    QVariantList out;
    const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames();
    for (const QString &name : services) {
        if (!name.startsWith(QLatin1String(kMprisPrefix)))
            continue;
        QVariantMap m;
        m[QStringLiteral("service")] = name;
        m[QStringLiteral("name")] = identityFor(name);
        m[QStringLiteral("current")] = (name == d->serviceName);
        m[QStringLiteral("spotify")] = name.contains(QLatin1String("spotify"), Qt::CaseInsensitive);
        out.push_back(m);
    }
    d->players = out;
    emit playersChanged();
}

QVariantList MprisController::players() const { return d->players; }
QString MprisController::currentService() const { return d->serviceName; }

void MprisController::selectPlayer(const QString &serviceName)
{
    if (serviceName.isEmpty())
        return;
    qCInfo(lcMpris) << "user selected" << serviceName;
    d->userPinned = true;
    attachToPlayer(serviceName);
    rebuildPlayerList();
}

void MprisController::findExistingPlayer()
{
    const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames();
    QString preferred;
    QString fallback;
    for (const QString &name : services) {
        if (!name.startsWith(QLatin1String(kMprisPrefix)))
            continue;
        if (d->userPinned && name == d->serviceName) {
            preferred = name;
            break;
        }
        if (name.contains(QLatin1String("spotifyd"), Qt::CaseInsensitive)
            || name.contains(QLatin1String("spotify"), Qt::CaseInsensitive)) {
            preferred = name;
            break;
        }
        if (fallback.isEmpty())
            fallback = name;
    }
    const QString chosen = !preferred.isEmpty() ? preferred : fallback;
    qCInfo(lcMpris) << "findExistingPlayer preferred" << preferred << "fallback" << fallback
                    << "pinned" << d->userPinned;
    if (!chosen.isEmpty())
        attachToPlayer(chosen);
    rebuildPlayerList();
}

void MprisController::onNameOwnerChanged(const QString &name, const QString &oldOwner,
                                          const QString &newOwner)
{
    if (!name.startsWith(QLatin1String(kMprisPrefix)))
        return;

    const bool appearing = oldOwner.isEmpty() && !newOwner.isEmpty();
    const bool vanishing = !oldOwner.isEmpty() && newOwner.isEmpty();

    if (vanishing && name == d->serviceName) {
        qCInfo(lcMpris) << "player vanished" << name;
        d->userPinned = false;
        detachPlayer();
        findExistingPlayer();
        return;
    }

    if (appearing) {
        rebuildPlayerList();
        if (d->userPinned || !d->serviceName.isEmpty())
            return;
        qCInfo(lcMpris) << "player appeared" << name << "attaching";
        attachToPlayer(name);
        return;
    }
    if (vanishing)
        rebuildPlayerList();
}

void MprisController::attachToPlayer(const QString &serviceName)
{
    if (d->serviceName == serviceName)
        return;
    detachPlayer();

    d->serviceName = serviceName;
    qCInfo(lcMpris) << "attached to" << serviceName;
    QDBusConnection bus = QDBusConnection::sessionBus();
    d->props = new QDBusInterface(serviceName, QString::fromLatin1(kMprisPath),
                                   QString::fromLatin1(kPropsIface), bus, this);
    d->player = new QDBusInterface(serviceName, QString::fromLatin1(kMprisPath),
                                    QString::fromLatin1(kPlayerIface), bus, this);

    // Metadata and PlaybackStatus are pushed here; Position is not (MPRIS has
    // no continuous-progress signal), so it alone is polled below.
    bus.connect(serviceName, QString::fromLatin1(kMprisPath), QString::fromLatin1(kPropsIface),
                QStringLiteral("PropertiesChanged"), this, SLOT(refreshMetadata()));

    refreshMetadata();
    emit availableChanged();
    rebuildPlayerList();
}

void MprisController::detachPlayer()
{
    if (d->serviceName.isEmpty())
        return;
    QDBusConnection::sessionBus().disconnect(
        d->serviceName, QString::fromLatin1(kMprisPath), QString::fromLatin1(kPropsIface),
        QStringLiteral("PropertiesChanged"), this, SLOT(refreshMetadata()));

    delete d->props;
    delete d->player;
    d->props = nullptr;
    d->player = nullptr;
    d->serviceName.clear();
    d->isPlaying = false;
    d->title.clear();
    d->artist.clear();
    d->album.clear();
    d->artUrl.clear();
    d->trackId.clear();
    d->lengthUs = 0;
    d->positionTimer.stop();
    d->positionRatio = 0.0;
    d->positionLabel = QStringLiteral("0:00");
    d->durationLabel = QStringLiteral("0:00");

    emit availableChanged();
    emit playbackChanged();
    emit positionChanged();
}

void MprisController::refreshMetadata()
{
    if (!d->props || !d->player)
        return;

    QDBusReply<QVariant> statusReply = d->props->call(QStringLiteral("Get"),
                                                        QString::fromLatin1(kPlayerIface),
                                                        QStringLiteral("PlaybackStatus"));
    const QString status = statusReply.isValid() ? statusReply.value().toString() : QString();
    d->isPlaying = status.compare(QLatin1String("Playing"), Qt::CaseInsensitive) == 0;

    QDBusReply<QVariant> metaReply = d->props->call(QStringLiteral("Get"),
                                                      QString::fromLatin1(kPlayerIface),
                                                      QStringLiteral("Metadata"));
    if (metaReply.isValid()) {
        const QVariantMap meta = demarshalMap(metaReply.value());
        d->title = meta.value(QStringLiteral("xesam:title")).toString();
        const QStringList artists = meta.value(QStringLiteral("xesam:artist")).toStringList();
        d->artist = artists.join(QStringLiteral(", "));
        d->album = meta.value(QStringLiteral("xesam:album")).toString();
        d->artUrl = meta.value(QStringLiteral("mpris:artUrl")).toString();
        d->trackId = meta.value(QStringLiteral("mpris:trackid")).toString();
        d->lengthUs = meta.value(QStringLiteral("mpris:length")).toLongLong();
        d->durationLabel = formatMs(d->lengthUs / 1000);
    }
    qCInfo(lcMpris) << "metadata" << status << d->title << d->artist;

    if (d->isPlaying)
        d->positionTimer.start();
    else
        d->positionTimer.stop();

    refreshPosition();
    emit playbackChanged();
}

void MprisController::refreshPosition()
{
    if (!d->props) {
        if (d->positionRatio != 0.0 || d->positionLabel != QStringLiteral("0:00")) {
            d->positionRatio = 0.0;
            d->positionLabel = QStringLiteral("0:00");
            emit positionChanged();
        }
        return;
    }

    QDBusReply<QVariant> posReply = d->props->call(QStringLiteral("Get"),
                                                     QString::fromLatin1(kPlayerIface),
                                                     QStringLiteral("Position"));
    const qint64 positionUs = posReply.isValid() ? posReply.value().toLongLong() : 0;
    d->positionLabel = formatMs(positionUs / 1000);
    d->positionRatio = d->lengthUs > 0
        ? qBound(0.0, static_cast<double>(positionUs) / static_cast<double>(d->lengthUs), 1.0)
        : 0.0;
    emit positionChanged();
}

bool MprisController::available() const
{
    return d->player != nullptr || (d->api && d->api->controllingRemote());
}
bool MprisController::isPlaying() const
{
    return (d->api && d->api->controllingRemote()) ? d->api->remotePlaying() : d->isPlaying;
}
QString MprisController::title() const
{
    return (d->api && d->api->controllingRemote()) ? d->api->remoteTitle() : d->title;
}
QString MprisController::artist() const
{
    return (d->api && d->api->controllingRemote()) ? d->api->remoteArtist() : d->artist;
}
QString MprisController::album() const { return d->album; }
QString MprisController::artUrl() const
{
    return (d->api && d->api->controllingRemote()) ? d->api->remoteArtUrl() : d->artUrl;
}

QString MprisController::nowPlayingLabel() const
{
    const QString t = title();
    if (t.isEmpty())
        return QString();
    const QString a = artist();
    return a.isEmpty() ? t : (t + QStringLiteral(" — ") + a);
}

double MprisController::positionRatio() const
{
    return (d->api && d->api->controllingRemote()) ? d->api->remotePositionRatio() : d->positionRatio;
}
QString MprisController::positionLabel() const
{
    return (d->api && d->api->controllingRemote()) ? d->api->remotePositionLabel() : d->positionLabel;
}
QString MprisController::durationLabel() const
{
    return (d->api && d->api->controllingRemote()) ? d->api->remoteDurationLabel() : d->durationLabel;
}

bool MprisController::isSpotifyPlayer() const
{
    if (d->api && d->api->controllingRemote())
        return true;
    if (d->serviceName.contains(QLatin1String("spotify"), Qt::CaseInsensitive))
        return true;
    const QString url = artUrl();
    return url.contains(QLatin1String("scdn.co")) || url.contains(QLatin1String("spotify"));
}

void MprisController::togglePlayPause()
{
    if (d->api && d->api->controllingRemote()) {
        d->api->togglePlayback();
        return;
    }
    qCInfo(lcMpris) << "PlayPause available=" << (d->player != nullptr) << "playing=" << d->isPlaying;
    if (d->player)
        d->player->asyncCall(QStringLiteral("PlayPause"));
}

void MprisController::next()
{
    if (d->api && d->api->controllingRemote()) {
        d->api->skipNext();
        return;
    }
    qCInfo(lcMpris) << "Next";
    if (d->player)
        d->player->asyncCall(QStringLiteral("Next"));
}

void MprisController::previous()
{
    if (d->api && d->api->controllingRemote()) {
        d->api->skipPrevious();
        return;
    }
    qCInfo(lcMpris) << "Previous";
    if (d->player)
        d->player->asyncCall(QStringLiteral("Previous"));
}

void MprisController::seekToRatio(double ratio)
{
    if (!d->player || d->lengthUs <= 0)
        return;
    ratio = qBound(0.0, ratio, 1.0);
    const qint64 targetUs = static_cast<qint64>(ratio * d->lengthUs);

    // SetPosition needs the current track's object path; without one (a
    // player that doesn't report mpris:trackid) fall back to a relative Seek
    // from the last known position, which is coarser but still functional.
    if (!d->trackId.isEmpty()) {
        d->player->asyncCall(QStringLiteral("SetPosition"),
                              QVariant::fromValue(QDBusObjectPath(d->trackId)), targetUs);
    } else {
        const qint64 currentUs = static_cast<qint64>(d->positionRatio * d->lengthUs);
        d->player->asyncCall(QStringLiteral("Seek"), targetUs - currentUs);
    }
    // Reflect the seek immediately rather than waiting for the next poll tick.
    d->positionRatio = ratio;
    d->positionLabel = formatMs(targetUs / 1000);
    emit positionChanged();
}

} // namespace polomodoro
