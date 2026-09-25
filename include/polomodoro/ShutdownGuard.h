#pragma once

#include <QObject>
#include <memory>

namespace polomodoro {

class DatabaseManager;
class TaskTree;

class ShutdownGuard : public QObject {
    Q_OBJECT
public:
    ShutdownGuard(DatabaseManager &db, TaskTree &tree, QObject *parent = nullptr);
    ~ShutdownGuard();

    void startHeartbeat(int intervalMs = 10000);

private slots:
    // Targeted by QDBusConnection::connect's string-based SLOT() API, which
    // requires a real moc-registered slot.
    void onPrepareForShutdown(bool starting);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
