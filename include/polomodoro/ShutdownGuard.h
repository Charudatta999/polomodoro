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

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
