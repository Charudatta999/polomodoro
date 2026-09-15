#pragma once

#include <memory>
#include <QString>

class QSqlDatabase;

namespace polomodoro {

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    bool open();
    void close();
    bool isOpen() const;

    bool beginTransaction();
    bool commit();
    bool rollback();
    void checkpointWal();

    QSqlDatabase database() const;

    bool setAppState(const QString &key, const QString &value);
    QString appState(const QString &key, const QString &defaultValue = {}) const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
