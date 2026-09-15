#include "polomodoro/DatabaseManager.h"

#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace polomodoro {

struct DatabaseManager::Impl {
    QSqlDatabase db;
    QString connectionName;
};

DatabaseManager::DatabaseManager() : d(std::make_unique<Impl>()) {}

DatabaseManager::~DatabaseManager()
{
    if (d->db.isOpen())
        d->db.close();
    // Skip removeDatabase — other Qt SQL handles may still exist at teardown.
}

bool DatabaseManager::open()
{
    if (d->db.isOpen())
        return true;

    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/polomodoro");
    QDir().mkpath(configDir);

    d->connectionName = QStringLiteral("polomodoro_%1").arg(reinterpret_cast<quintptr>(this));
    d->db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), d->connectionName);
    d->db.setDatabaseName(configDir + QStringLiteral("/polomodoro.db"));

    if (!d->db.open()) {
        return false;
    }

    QSqlQuery pragma(d->db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));

    QSqlQuery versionQuery(d->db);
    versionQuery.exec(QStringLiteral("SELECT version FROM schema_version LIMIT 1"));
    if (!versionQuery.next()) {
        QFile schemaFile(QStringLiteral(":/sql/001_initial.sql"));
        if (!schemaFile.open(QIODevice::ReadOnly))
            return false;
        const QString sql = QString::fromUtf8(schemaFile.readAll());
        const auto statements = sql.split(';', Qt::SkipEmptyParts);
        for (const QString &stmt : statements) {
            const QString trimmed = stmt.trimmed();
            if (trimmed.isEmpty())
                continue;
            QSqlQuery q(d->db);
            if (!q.exec(trimmed))
                return false;
        }
        QSqlQuery insertVersion(d->db);
        insertVersion.prepare(QStringLiteral("INSERT INTO schema_version(version) VALUES (?)"));
        insertVersion.addBindValue(1);
        insertVersion.exec();
    }

    return true;
}

void DatabaseManager::close()
{
    if (d->db.isOpen())
        d->db.close();
}

bool DatabaseManager::isOpen() const
{
    return d->db.isOpen();
}

bool DatabaseManager::beginTransaction()
{
    return d->db.transaction();
}

bool DatabaseManager::commit()
{
    return d->db.commit();
}

bool DatabaseManager::rollback()
{
    return d->db.rollback();
}

void DatabaseManager::checkpointWal()
{
    QSqlQuery q(d->db);
    q.exec(QStringLiteral("PRAGMA wal_checkpoint(FULL)"));
}

QSqlDatabase DatabaseManager::database() const
{
    return d->db;
}

bool DatabaseManager::setAppState(const QString &key, const QString &value)
{
    QSqlQuery q(d->db);
    q.prepare(QStringLiteral("INSERT INTO app_state(key, value) VALUES(?, ?) "
                             "ON CONFLICT(key) DO UPDATE SET value=excluded.value"));
    q.addBindValue(key);
    q.addBindValue(value);
    return q.exec();
}

QString DatabaseManager::appState(const QString &key, const QString &defaultValue) const
{
    QSqlQuery q(d->db);
    q.prepare(QStringLiteral("SELECT value FROM app_state WHERE key=?"));
    q.addBindValue(key);
    if (q.exec() && q.next())
        return q.value(0).toString();
    return defaultValue;
}

} // namespace polomodoro
