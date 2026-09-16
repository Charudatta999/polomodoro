#include "polomodoro/NotificationManager.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QStringList>
#include <QVariantMap>

namespace polomodoro {

NotificationManager::NotificationManager(QObject *parent) : QObject(parent)
{
    m_interface = new QDBusInterface(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QDBusConnection::sessionBus(), this);
}

NotificationManager::~NotificationManager() = default;

void NotificationManager::notify(const QString &summary, const QString &body)
{
    if (!m_interface || !m_interface->isValid())
        return;

    const QDBusReply<uint> reply = m_interface->call(
        QStringLiteral("Notify"),
        QStringLiteral("Polomodoro"),
        uint(0),
        QStringLiteral("appointment-soon"),
        summary,
        body,
        QStringList(),
        QVariantMap(),
        int(5000));
    if (reply.isValid())
        m_lastId = reply.value();
}

} // namespace polomodoro
