#pragma once

#include <QObject>
#include <memory>

namespace polomodoro {

class SettingsStore;

// Spawns and owns a dedicated spotifyd child process (own config/cache dir,
// own device name — see settings spotifyDeviceName) so Polomodoro doesn't
// depend on the user starting one manually. A no-op if spotifyAutoLaunch is
// off, the binary isn't found, or a spotify-named MPRIS player is already on
// the bus (never launch a second one on top of the user's own).
class SpotifydManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(bool binaryFound READ binaryFound CONSTANT)
public:
    explicit SpotifydManager(SettingsStore &settings, QObject *parent = nullptr);
    ~SpotifydManager() override;

    bool isRunning() const;
    bool binaryFound() const;

    // Stops and relaunches — exposed for Settings, e.g. after the user
    // changes the device name or flips auto-launch back on.
    Q_INVOKABLE void restart();

signals:
    void runningChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> d;

    void start();
    void stop();
};

} // namespace polomodoro
