#include "polomodoro/ShutdownGuard.h"

#include "polomodoro/DatabaseManager.h"
#include "polomodoro/TaskTree.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QTimer>

namespace polomodoro {

struct ShutdownGuard::Impl {
    DatabaseManager *db = nullptr;
    TaskTree *tree = nullptr;
    QTimer heartbeat;

    void flush()
    {
        db->beginTransaction();
        tree->saveAll();
        tree->heartbeat();
        db->commit();
        db->checkpointWal();
    }
};

ShutdownGuard::ShutdownGuard(DatabaseManager &db, TaskTree &tree, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>())
{
    d->db = &db;
    d->tree = &tree;
    connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit, this, [this]() { d->flush(); });

    // A system suspend/shutdown/reboot kills the process before aboutToQuit
    // fires in many desktop environments; logind's PrepareForShutdown signal
    // is the only reliable hook to flush ahead of that.
    QDBusConnection::systemBus().connect(
        QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"),
        QStringLiteral("org.freedesktop.login1.Manager"), QStringLiteral("PrepareForShutdown"), this,
        SLOT(onPrepareForShutdown(bool)));
}

void ShutdownGuard::onPrepareForShutdown(bool starting)
{
    if (starting)
        d->flush();
}

ShutdownGuard::~ShutdownGuard() = default;

void ShutdownGuard::startHeartbeat(int intervalMs)
{
    d->heartbeat.setInterval(intervalMs);
    connect(&d->heartbeat, &QTimer::timeout, this, [this]() {
        if (!d->tree->anyTaskActive())
            return;
        d->db->beginTransaction();
        d->tree->saveAll();
        d->tree->heartbeat();
        d->db->commit();
    });
    d->heartbeat.start();
}

} // namespace polomodoro
