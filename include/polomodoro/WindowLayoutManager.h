#pragma once

#include <QObject>
#include <memory>

namespace polomodoro {

class SettingsStore;

class WindowLayoutManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(bool alwaysOnTop READ alwaysOnTop WRITE setAlwaysOnTop NOTIFY alwaysOnTopChanged)
    Q_PROPERTY(bool barDropdownExpanded READ barDropdownExpanded WRITE setBarDropdownExpanded NOTIFY barDropdownExpandedChanged)
public:
    explicit WindowLayoutManager(SettingsStore &settings, QObject *parent = nullptr);
    ~WindowLayoutManager();

    QString viewMode() const;
    bool alwaysOnTop() const;
    bool barDropdownExpanded() const;

    Q_INVOKABLE void setViewMode(const QString &mode);
    Q_INVOKABLE void cycleViewMode();
    Q_INVOKABLE void saveGeometry(const QString &mode, int x, int y, int w, int h);
    Q_INVOKABLE QVariantList geometryForMode(const QString &mode) const;

public slots:
    void setAlwaysOnTop(bool value);
    void setBarDropdownExpanded(bool value);

signals:
    void viewModeChanged();
    void alwaysOnTopChanged();
    void barDropdownExpandedChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
