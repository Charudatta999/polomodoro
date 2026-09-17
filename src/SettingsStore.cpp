#include "polomodoro/SettingsStore.h"

#include "polomodoro/DatabaseManager.h"

#include <QSqlError>
#include <QSqlQuery>

namespace polomodoro {

struct SettingsStore::Impl {
    DatabaseManager &db;
};

SettingsStore::SettingsStore(DatabaseManager &db) : d(std::make_unique<Impl>(Impl{db})) {}
SettingsStore::~SettingsStore() = default;

void SettingsStore::seedDefaults()
{
    const auto ensure = [this](const QString &key, const QString &value) {
        QSqlQuery q(d->db.database());
        q.prepare(QStringLiteral("INSERT OR IGNORE INTO settings(key, value) VALUES(?, ?)"));
        q.addBindValue(key);
        // A null QString() binds as SQL NULL, not "", and `value` is NOT
        // NULL — that insert fails and INSERT OR IGNORE swallows it, so an
        // empty-string default (backgroundUserPath, spotifyClientId,
        // spotifyDeviceName) would silently never get seeded.
        q.addBindValue(value.isNull() ? QStringLiteral("") : value);
        if (!q.exec())
            qWarning("SettingsStore::seedDefaults: failed to seed '%s': %s",
                     qUtf8Printable(key), qUtf8Printable(q.lastError().text()));
    };

    ensure(QStringLiteral("pomodoroWorkMs"), QStringLiteral("1500000"));
    ensure(QStringLiteral("pomodoroShortBreakMs"), QStringLiteral("300000"));
    ensure(QStringLiteral("pomodoroLongBreakMs"), QStringLiteral("900000"));
    ensure(QStringLiteral("pomodoroCyclesBeforeLongBreak"), QStringLiteral("4"));
    ensure(QStringLiteral("backgroundRotationSec"), QStringLiteral("300"));
    ensure(QStringLiteral("backgroundUserPath"), QString());
    ensure(QStringLiteral("backgroundSource"), QStringLiteral("wallpaper"));
    ensure(QStringLiteral("spotifyArtFallbackToWallpaper"), QStringLiteral("true"));
    ensure(QStringLiteral("alwaysOnTop"), QStringLiteral("false"));
    ensure(QStringLiteral("viewMode"), QStringLiteral("expanded"));
    ensure(QStringLiteral("pipAutoAlwaysOnTop"), QStringLiteral("true"));
    ensure(QStringLiteral("barAutoAlwaysOnTop"), QStringLiteral("true"));
    ensure(QStringLiteral("barDropdownExpanded"), QStringLiteral("false"));
    ensure(QStringLiteral("autoCompactOnResize"), QStringLiteral("false"));
    ensure(QStringLiteral("expandedGeometry"), QStringLiteral("-1,-1,1280,800"));
    ensure(QStringLiteral("barGeometry"), QStringLiteral("-1,-1,800,48"));
    ensure(QStringLiteral("compactGeometry"), QStringLiteral("-1,-1,300,140"));
    ensure(QStringLiteral("defaultTargetMs"), QStringLiteral("0"));
    ensure(QStringLiteral("targetProgressBasis"), QStringLiteral("active"));
    ensure(QStringLiteral("notifyOnTargetReached"), QStringLiteral("true"));
    ensure(QStringLiteral("notifyOnTaskStart"), QStringLiteral("true"));
    ensure(QStringLiteral("notifyOnEndDateApproaching"), QStringLiteral("true"));
    ensure(QStringLiteral("showDayTimeline"), QStringLiteral("true"));
    ensure(QStringLiteral("accentMode"), QStringLiteral("auto"));

    // Spotify Web API (PKCE) — client id is public by design (no secret
    // ships), but there is no default that works for every install, so none
    // is invented here; Settings prompts for one. The refresh token itself
    // never goes here — see SpotifyWebApi, which uses the system keyring.
    ensure(QStringLiteral("spotifyClientId"), QString());
    ensure(QStringLiteral("spotifyRedirectPort"), QStringLiteral("8888"));
    // Unlike the client id, this is just a Spotify Connect device label, not
    // an identity — safe to default now that Polomodoro owns launching the
    // process that uses it (SpotifydManager).
    ensure(QStringLiteral("spotifyDeviceName"), QStringLiteral("Polomodoro"));
    ensure(QStringLiteral("spotifyAutoLaunch"), QStringLiteral("true"));
    ensure(QStringLiteral("nowPlayingStripVisible"), QStringLiteral("true"));
    ensure(QStringLiteral("libraryOverlayWidth"), QStringLiteral("520"));
}

QString SettingsStore::getString(const QString &key, const QString &defaultValue) const
{
    QSqlQuery q(d->db.database());
    q.prepare(QStringLiteral("SELECT value FROM settings WHERE key=?"));
    q.addBindValue(key);
    if (q.exec() && q.next())
        return q.value(0).toString();
    return defaultValue;
}

int SettingsStore::getInt(const QString &key, int defaultValue) const
{
    const QString v = getString(key);
    return v.isEmpty() ? defaultValue : v.toInt();
}

bool SettingsStore::getBool(const QString &key, bool defaultValue) const
{
    const QString v = getString(key);
    if (v.isEmpty())
        return defaultValue;
    return v == QStringLiteral("true") || v == QStringLiteral("1");
}

void SettingsStore::setString(const QString &key, const QString &value)
{
    QSqlQuery q(d->db.database());
    q.prepare(QStringLiteral("INSERT INTO settings(key, value) VALUES(?, ?) "
                             "ON CONFLICT(key) DO UPDATE SET value=excluded.value"));
    q.addBindValue(key);
    q.addBindValue(value);
    q.exec();
}

void SettingsStore::setInt(const QString &key, int value)
{
    setString(key, QString::number(value));
}

void SettingsStore::setBool(const QString &key, bool value)
{
    setString(key, value ? QStringLiteral("true") : QStringLiteral("false"));
}

} // namespace polomodoro
