#pragma once

#include <QObject>
#include <memory>

namespace polomodoro {

class BackgroundManager;
class SettingsStore;

class BackgroundController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentImageUrl READ currentImageUrl NOTIFY backgroundChanged)
    Q_PROPERTY(QString backgroundSource READ backgroundSource WRITE setBackgroundSource NOTIFY backgroundChanged)
    Q_PROPERTY(QString phaseTint READ phaseTint WRITE setPhaseTint NOTIFY backgroundChanged)
public:
    BackgroundController(BackgroundManager &manager, SettingsStore &settings, QObject *parent = nullptr);
    ~BackgroundController();

    QString currentImageUrl() const;
    QString backgroundSource() const;
    QString phaseTint() const;

    void setBackgroundSource(const QString &source);
    void setPhaseTint(const QString &tint);
    Q_INVOKABLE void cycleBackgroundSource();
    Q_INVOKABLE void tick();

signals:
    void backgroundChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
