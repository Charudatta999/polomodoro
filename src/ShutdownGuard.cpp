#include "polomodoro/ShutdownGuard.h"

#include "polomodoro/DatabaseManager.h"
#include "polomodoro/TaskTree.h"

#include <QGuiApplication>
#include <QTimer>

namespace polomodoro {

struct ShutdownGuard::Impl {
    DatabaseManager *db = nullptr;
    TaskTree *tree = nullptr;
    QTimer heartbeat;
};

ShutdownGuard::ShutdownGuard(DatabaseManager &db, TaskTree &tree, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>())
{
    d->db = &db;
    d->tree = &tree;
    connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit, this, [this]() {
        d->db->beginTransaction();
        d->tree->saveAll();
        d->tree->heartbeat();
        d->db->commit();
        d->db->checkpointWal();
    });

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
