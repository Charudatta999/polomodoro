#include "polomodoro/SpotifydManager.h"

#include "polomodoro/SettingsStore.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>

namespace polomodoro {

namespace {
constexpr const char *kMprisPrefix = "org.mpris.MediaPlayer2.";

bool spotifyPlayerAlreadyOnBus()
{
    const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames();
    for (const QString &name : services) {
        if (name.startsWith(QLatin1String(kMprisPrefix)) && name.contains(QLatin1String("spotify"), Qt::CaseInsensitive))
            return true;
    }
    return false;
}
} // namespace

struct SpotifydManager::Impl {
    SettingsStore &settings;
    QString binaryPath;
    std::unique_ptr<QProcess> process;
};

SpotifydManager::SpotifydManager(SettingsStore &settings, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>(Impl{settings}))
{
    d->binaryPath = QStandardPaths::findExecutable(QStringLiteral("spotifyd"));

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() { stop(); });

    if (!binaryFound()) {
        qWarning("SpotifydManager: spotifyd not found on PATH — install it "
                 "(see README) for automatic playback; MPRIS transport still "
                 "works with any other player already running.");
        return;
    }
    if (!d->settings.getBool(QStringLiteral("spotifyAutoLaunch"), true))
        return;
    if (spotifyPlayerAlreadyOnBus()) {
        qInfo("SpotifydManager: a spotify MPRIS player is already on the bus — not launching a second one.");
        return;
    }
    start();
}

SpotifydManager::~SpotifydManager()
{
    stop();
}

bool SpotifydManager::isRunning() const
{
    return d->process && d->process->state() != QProcess::NotRunning;
}

bool SpotifydManager::binaryFound() const
{
    return !d->binaryPath.isEmpty();
}

void SpotifydManager::start()
{
    if (isRunning() || !binaryFound())
        return;

    // Own config/cache dir, separate from any system-wide ~/.config/spotifyd
    // a user might already have — this instance never touches that one.
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/polomodoro/spotifyd");
    QDir().mkpath(dataDir);

    QString deviceName = d->settings.getString(QStringLiteral("spotifyDeviceName")).trimmed();
    if (deviceName.isEmpty())
        deviceName = QStringLiteral("Polomodoro");

    QStringList args{
        QStringLiteral("--no-daemon"), // stay attached to us as a real child; spotifyd forks+detaches by default
        QStringLiteral("--cache-path"), dataDir,
        QStringLiteral("--device-name"), deviceName,
        QStringLiteral("--dbus-type"), QStringLiteral("session"), // MprisController watches the session bus
    };

    d->process = std::make_unique<QProcess>();
    d->process->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(d->process.get(), &QProcess::stateChanged, this, [this](QProcess::ProcessState) { emit runningChanged(); });
    connect(d->process.get(), &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        qWarning("SpotifydManager: spotifyd process error: %s", qUtf8Printable(d->process->errorString()));
    });
    connect(d->process.get(), &QProcess::finished, this,
            [](int exitCode, QProcess::ExitStatus status) {
                if (status != QProcess::NormalExit || exitCode != 0)
                    qWarning("SpotifydManager: spotifyd exited abnormally (code %d)", exitCode);
            });

    d->process->start(d->binaryPath, args);
}

void SpotifydManager::stop()
{
    if (!d->process)
        return;
    if (d->process->state() != QProcess::NotRunning) {
        d->process->terminate();
        if (!d->process->waitForFinished(3000))
            d->process->kill();
    }
    d->process.reset();
}

void SpotifydManager::restart()
{
    stop();
    start();
}

} // namespace polomodoro
