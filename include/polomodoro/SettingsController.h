#pragma once

#include <QObject>
#include <QVariantList>
#include <memory>

namespace polomodoro {

class SettingsStore;

class SettingsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool alwaysOnTop READ alwaysOnTop WRITE setAlwaysOnTop NOTIFY settingsChanged)
    Q_PROPERTY(QString backgroundSource READ backgroundSource WRITE setBackgroundSource NOTIFY settingsChanged)
    Q_PROPERTY(int backgroundRotationSec READ backgroundRotationSec WRITE setBackgroundRotationSec NOTIFY settingsChanged)
    Q_PROPERTY(bool notifyOnTargetReached READ notifyOnTargetReached WRITE setNotifyOnTargetReached NOTIFY settingsChanged)
    Q_PROPERTY(int cyclesBeforeLongBreak READ cyclesBeforeLongBreak NOTIFY settingsChanged)
    Q_PROPERTY(bool showDayTimeline READ showDayTimeline WRITE setShowDayTimeline NOTIFY settingsChanged)
    Q_PROPERTY(bool barDropdownExpanded READ barDropdownExpanded WRITE setBarDropdownExpanded NOTIFY settingsChanged)
    Q_PROPERTY(int workMinutes READ workMinutes WRITE setWorkMinutes NOTIFY settingsChanged)
    Q_PROPERTY(int shortBreakMinutes READ shortBreakMinutes WRITE setShortBreakMinutes NOTIFY settingsChanged)
    Q_PROPERTY(int longBreakMinutes READ longBreakMinutes WRITE setLongBreakMinutes NOTIFY settingsChanged)
    Q_PROPERTY(int rotationSec READ rotationSec WRITE setRotationSec NOTIFY settingsChanged)
    Q_PROPERTY(QString progressBasis READ progressBasis WRITE setProgressBasis NOTIFY settingsChanged)
    Q_PROPERTY(QString accentMode READ accentMode WRITE setAccentMode NOTIFY settingsChanged)
    Q_PROPERTY(QString accentManualPalette READ accentManualPalette WRITE setAccentManualPalette NOTIFY settingsChanged)
    Q_PROPERTY(QVariantList palettePresets READ palettePresets CONSTANT)
    Q_PROPERTY(QString integrityReport READ integrityReport NOTIFY settingsChanged)
    Q_PROPERTY(bool nowPlayingStripVisible READ nowPlayingStripVisible WRITE setNowPlayingStripVisible NOTIFY settingsChanged)
    Q_PROPERTY(int libraryOverlayWidth READ libraryOverlayWidth NOTIFY settingsChanged)
    Q_PROPERTY(QString spotifyClientId READ spotifyClientId WRITE setSpotifyClientId NOTIFY settingsChanged)
    Q_PROPERTY(QString spotifyDeviceName READ spotifyDeviceName WRITE setSpotifyDeviceName NOTIFY settingsChanged)
    Q_PROPERTY(bool spotifyAutoLaunch READ spotifyAutoLaunch WRITE setSpotifyAutoLaunch NOTIFY settingsChanged)
public:
    explicit SettingsController(SettingsStore &store, QObject *parent = nullptr);
    ~SettingsController();

    bool alwaysOnTop() const;
    QString backgroundSource() const;
    int backgroundRotationSec() const;
    bool notifyOnTargetReached() const;
    int cyclesBeforeLongBreak() const;
    bool showDayTimeline() const;
    bool barDropdownExpanded() const;
    int workMinutes() const;
    int shortBreakMinutes() const;
    int longBreakMinutes() const;
    int rotationSec() const;
    QString progressBasis() const;
    QString accentMode() const;
    QString accentManualPalette() const;
    QVariantList palettePresets() const;
    QString integrityReport() const;
    bool nowPlayingStripVisible() const;
    int libraryOverlayWidth() const;
    QString spotifyClientId() const;
    QString spotifyDeviceName() const;
    bool spotifyAutoLaunch() const;

    void setAlwaysOnTop(bool value);
    void setBackgroundSource(const QString &value);
    void setBackgroundRotationSec(int value);
    void setNotifyOnTargetReached(bool value);
    void setShowDayTimeline(bool value);
    void setBarDropdownExpanded(bool value);
    void setWorkMinutes(int value);
    void setShortBreakMinutes(int value);
    void setLongBreakMinutes(int value);
    void setRotationSec(int value);
    void setProgressBasis(const QString &value);
    void setAccentMode(const QString &value);
    void setAccentManualPalette(const QString &value);
    void setNowPlayingStripVisible(bool value);
    void setSpotifyClientId(const QString &value);
    void setSpotifyDeviceName(const QString &value);
    void setSpotifyAutoLaunch(bool value);

    Q_INVOKABLE int pomodoroWorkMs() const;
    Q_INVOKABLE int pomodoroShortBreakMs() const;
    Q_INVOKABLE int pomodoroLongBreakMs() const;
    Q_INVOKABLE void exportCsv();

signals:
    void settingsChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace polomodoro
