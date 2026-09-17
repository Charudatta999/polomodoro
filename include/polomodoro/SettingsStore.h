#pragma once

#include <memory>
#include <QString>

namespace polomodoro {

class DatabaseManager;

class SettingsStore {
public:
    explicit SettingsStore(DatabaseManager &db);
    ~SettingsStore();

    void seedDefaults();

    QString getString(const QString &key, const QString &defaultValue = {}) const;
    int getInt(const QString &key, int defaultValue = 0) const;
    bool getBool(const QString &key, bool defaultValue = false) const;

    void setString(const QString &key, const QString &value);
    void setInt(const QString &key, int value);
    void setBool(const QString &key, bool value);

    QString integrityReport() const;
    bool exportSessionsCsv(const QString &path, QString *error) const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
