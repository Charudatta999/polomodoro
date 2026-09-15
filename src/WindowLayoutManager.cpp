#include "polomodoro/WindowLayoutManager.h"

#include "polomodoro/SettingsStore.h"

#include <QVariant>

namespace polomodoro {

struct WindowLayoutManager::Impl {
    SettingsStore &settings;
    QString viewMode;
    bool alwaysOnTop = false;
    bool barDropdownExpanded = false;
};

WindowLayoutManager::WindowLayoutManager(SettingsStore &settings, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>(Impl{settings}))
{
    d->viewMode = settings.getString(QStringLiteral("viewMode"), QStringLiteral("expanded"));
    d->alwaysOnTop = settings.getBool(QStringLiteral("alwaysOnTop"));
    d->barDropdownExpanded = settings.getBool(QStringLiteral("barDropdownExpanded"));
}

WindowLayoutManager::~WindowLayoutManager() = default;

QString WindowLayoutManager::viewMode() const { return d->viewMode; }
bool WindowLayoutManager::alwaysOnTop() const { return d->alwaysOnTop; }
bool WindowLayoutManager::barDropdownExpanded() const { return d->barDropdownExpanded; }

void WindowLayoutManager::setViewMode(const QString &mode)
{
    if (d->viewMode == mode)
        return;
    d->viewMode = mode;
    d->settings.setString(QStringLiteral("viewMode"), mode);
    emit viewModeChanged();
}

void WindowLayoutManager::cycleViewMode()
{
    if (d->viewMode == QStringLiteral("expanded"))
        setViewMode(QStringLiteral("bar"));
    else if (d->viewMode == QStringLiteral("bar"))
        setViewMode(QStringLiteral("compact"));
    else
        setViewMode(QStringLiteral("expanded"));
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
        return {QVariant(-1), QVariant(-1), QVariant(1280), QVariant(800)};
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
