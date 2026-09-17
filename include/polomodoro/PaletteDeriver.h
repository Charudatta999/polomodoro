#pragma once

#include <QColor>
#include <QObject>
#include <memory>

namespace polomodoro {

class SettingsStore;

// Derives Theme.accent / accentHover / break / overflow / muted from the
// current background image (wallpaper or mpris:artUrl). Downscale 64×64,
// k-means in OKLab, clamp L and C per the spec. Manual presets skip this
// and apply the four named palettes instead.
class PaletteDeriver : public QObject {
    Q_OBJECT
    Q_PROPERTY(QColor accent READ accent NOTIFY paletteChanged)
    Q_PROPERTY(QColor accentHover READ accentHover NOTIFY paletteChanged)
    Q_PROPERTY(QColor breakColor READ breakColor NOTIFY paletteChanged)
    Q_PROPERTY(QColor overflow READ overflow NOTIFY paletteChanged)
    Q_PROPERTY(QColor muted READ muted NOTIFY paletteChanged)
public:
    explicit PaletteDeriver(SettingsStore &settings, QObject *parent = nullptr);
    ~PaletteDeriver() override;

    QColor accent() const;
    QColor accentHover() const;
    QColor breakColor() const;
    QColor overflow() const;
    QColor muted() const;

    Q_INVOKABLE void recompute(const QString &imageUrl);

signals:
    void paletteChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> d;

    void applyManual(const QString &presetId);
    void applyDerived(const QColor &accent);
    void applyForest();
};
} // namespace polomodoro
