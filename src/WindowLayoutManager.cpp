#include "polomodoro/WindowLayoutManager.h"

#include "polomodoro/SettingsStore.h"

#include <QVariant>

namespace polomodoro {

struct WindowLayoutManager::Impl {
    SettingsStore &settings;
    QString viewModeKey = QStringLiteral("expanded");
    bool alwaysOnTop = false;
    bool barDropdownExpanded = false;
    int x = 100;
    int y = 100;
    int width = 1280;
    int height = 800;
};

static QString modeKeyFromInt(int mode)
{
    switch (mode) {
    case 0: return QStringLiteral("expanded");
    case 1: return QStringLiteral("bar");
    default: return QStringLiteral("compact");
    }
}

static int modeIntFromKey(const QString &key)
{
    if (key == QStringLiteral("bar"))
        return 1;
    if (key == QStringLiteral("compact"))
        return 2;
    return 0;
}

static QVariantList defaultGeometryForMode(const QString &mode)
{
    if (mode == QStringLiteral("bar"))
        return {QVariant(100), QVariant(100), QVariant(800), QVariant(48)};
    if (mode == QStringLiteral("compact"))
        return {QVariant(100), QVariant(100), QVariant(300), QVariant(140)};
    return {QVariant(100), QVariant(100), QVariant(1280), QVariant(800)};
}

static bool isValidGeometryForMode(const QString &mode, int w, int h)
{
    if (mode == QStringLiteral("expanded"))
        return w >= 640 && h >= 420;
    if (mode == QStringLiteral("bar"))
        return w >= 560 && h >= 40 && h <= 300;
    if (mode == QStringLiteral("compact"))
        return w >= 260 && h >= 120 && w <= 500;
    return false;
}

WindowLayoutManager::WindowLayoutManager(SettingsStore &settings, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>(Impl{settings}))
{
    d->viewModeKey = settings.getString(QStringLiteral("viewMode"), QStringLiteral("expanded"));
    d->alwaysOnTop = settings.getBool(QStringLiteral("alwaysOnTop"));
    d->barDropdownExpanded = settings.getBool(QStringLiteral("barDropdownExpanded"));
    loadGeometryForCurrentMode();
}

WindowLayoutManager::~WindowLayoutManager() = default;

int WindowLayoutManager::viewMode() const { return modeIntFromKey(d->viewModeKey); }
bool WindowLayoutManager::alwaysOnTop() const { return d->alwaysOnTop; }
bool WindowLayoutManager::barDropdownExpanded() const { return d->barDropdownExpanded; }
int WindowLayoutManager::x() const { return d->x; }
int WindowLayoutManager::y() const { return d->y; }
int WindowLayoutManager::width() const { return d->width; }
int WindowLayoutManager::height() const { return d->height; }

void WindowLayoutManager::loadGeometryForCurrentMode()
{
    const QVariantList geo = geometryForMode(d->viewModeKey);
    if (geo.size() != 4)
        return;

    d->x = geo.at(0).toInt();
    d->y = geo.at(1).toInt();
    d->width = geo.at(2).toInt();
    d->height = geo.at(3).toInt();

    // Rewrite corrupted geometry saved before validation existed.
    const QString key = d->viewModeKey + QStringLiteral("Geometry");
    const QStringList parts = d->settings.getString(key).split(',');
    if (parts.size() == 4) {
        const int w = parts.at(2).toInt();
        const int h = parts.at(3).toInt();
        if (!isValidGeometryForMode(d->viewModeKey, w, h))
            saveGeometry(d->viewModeKey, d->x, d->y, d->width, d->height);
    }

    emit geometryChanged();
}

void WindowLayoutManager::setViewMode(int mode)
{
    setMode(mode);
}

void WindowLayoutManager::setMode(int mode)
{
    const QString key = modeKeyFromInt(mode);
    if (d->viewModeKey == key)
        return;
    d->viewModeKey = key;
    d->settings.setString(QStringLiteral("viewMode"), key);
    if (mode == 1 && d->settings.getBool(QStringLiteral("barAutoAlwaysOnTop"), true))
        setAlwaysOnTop(true);
    else if (mode == 2 && d->settings.getBool(QStringLiteral("pipAutoAlwaysOnTop"), true))
        setAlwaysOnTop(true);
    loadGeometryForCurrentMode();
    emit viewModeChanged();
}

void WindowLayoutManager::togglePip()
{
    setMode(viewMode() == 2 ? 0 : 2);
}

void WindowLayoutManager::rememberGeometry(int x, int y, int w, int h)
{
    // Persist only — updating properties here caused a width/height binding loop
    // with ApplicationWindow.
    saveGeometry(d->viewModeKey, x, y, w, h);
}

void WindowLayoutManager::setViewModeName(const QString &mode)
{
    setMode(modeIntFromKey(mode));
}

void WindowLayoutManager::cycleViewMode()
{
    setMode((viewMode() + 1) % 3);
}

void WindowLayoutManager::saveGeometry(const QString &mode, int x, int y, int w, int h)
{
    const QString key = mode + QStringLiteral("Geometry");
    d->settings.setString(key, QStringLiteral("%1,%2,%3,%4").arg(x).arg(y).arg(w).arg(h));
}

QVariantList WindowLayoutManager::geometryForMode(const QString &mode) const
{
    const QString key = mode + QStringLiteral("Geometry");
    const QStringList parts = d->settings.getString(key).split(',');
    if (parts.size() != 4)
        return defaultGeometryForMode(mode);

    const int w = parts.at(2).toInt();
    const int h = parts.at(3).toInt();
    if (!isValidGeometryForMode(mode, w, h))
        return defaultGeometryForMode(mode);

    QVariantList result;
    for (const QString &p : parts)
        result.push_back(QVariant(p.toInt()));
    return result;
}

void WindowLayoutManager::setAlwaysOnTop(bool value)
{
    if (d->alwaysOnTop == value)
        return;
    d->alwaysOnTop = value;
    d->settings.setBool(QStringLiteral("alwaysOnTop"), value);
    emit alwaysOnTopChanged();
}

void WindowLayoutManager::setBarDropdownExpanded(bool value)
{
    if (d->barDropdownExpanded == value)
        return;
    d->barDropdownExpanded = value;
    d->settings.setBool(QStringLiteral("barDropdownExpanded"), value);
    emit barDropdownExpandedChanged();
}

} // namespace polomodoro
