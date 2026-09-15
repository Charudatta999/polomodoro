#pragma once

#include <QObject>
#include <memory>

namespace polomodoro {

class SettingsStore;

class WindowLayoutManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(int viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(bool alwaysOnTop READ alwaysOnTop WRITE setAlwaysOnTop NOTIFY alwaysOnTopChanged)
    Q_PROPERTY(bool barDropdownExpanded READ barDropdownExpanded WRITE setBarDropdownExpanded NOTIFY barDropdownExpandedChanged)
    Q_PROPERTY(int x READ x NOTIFY geometryChanged)
    Q_PROPERTY(int y READ y NOTIFY geometryChanged)
    Q_PROPERTY(int width READ width NOTIFY geometryChanged)
    Q_PROPERTY(int height READ height NOTIFY geometryChanged)
public:
    explicit WindowLayoutManager(SettingsStore &settings, QObject *parent = nullptr);
    ~WindowLayoutManager();

    int viewMode() const;
    bool alwaysOnTop() const;
    bool barDropdownExpanded() const;
    int x() const;
    int y() const;
    int width() const;
    int height() const;

    void setViewMode(int mode);
    Q_INVOKABLE void setMode(int mode);
    Q_INVOKABLE void togglePip();
    Q_INVOKABLE void rememberGeometry(int x, int y, int w, int h);
    Q_INVOKABLE void setViewModeName(const QString &mode);
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
    void geometryChanged();

private:
    void loadGeometryForCurrentMode();

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
