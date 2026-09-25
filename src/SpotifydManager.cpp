#include "polomodoro/SpotifydManager.h"

#include "polomodoro/SettingsStore.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>

#include <signal.h>
#include <unistd.h>

namespace polomodoro {

Q_LOGGING_CATEGORY(lcSpotifyd, "polomodoro.spotifyd")

namespace {
constexpr const char *kMprisPrefix = "org.mpris.MediaPlayer2.";
constexpr quint16 kAuthRedirectPort = 8890;

bool spotifyPlayerAlreadyOnBus()
{
    const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames();
    for (const QString &name : services) {
        if (name.startsWith(QLatin1String(kMprisPrefix)) && name.contains(QLatin1String("spotify"), Qt::CaseInsensitive))
            return true;
    }
    return false;
}

bool cacheHasCredentialFiles(const QString &dir)
{
    if (QFile::exists(dir + QStringLiteral("/credentials.json")))
        return true;
    QDir oauth(dir + QStringLiteral("/oauth"));
    if (!oauth.exists())
        return false;
    const QStringList files = oauth.entryList(QDir::Files | QDir::NoDotAndDotDot);
    return !files.isEmpty();
}
} // namespace

struct SpotifydManager::Impl {
    SettingsStore &settings;
    QString binaryPath;
    std::unique_ptr<QProcess> process;
    std::unique_ptr<QProcess> authProcess;
    bool stopping = false;
    bool hasCredentials = false;
};

SpotifydManager::SpotifydManager(SettingsStore &settings, QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>(Impl{settings}))
{
    d->binaryPath = QStandardPaths::findExecutable(QStringLiteral("spotifyd"));
    qCInfo(lcSpotifyd) << "binary" << (d->binaryPath.isEmpty() ? QStringLiteral("(not found)") : d->binaryPath)
                       << "cache" << dataDir();

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() { stop(); });

    QDir().mkpath(dataDir());
    refreshCredentialsPresent();

    if (!binaryFound()) {
        qCWarning(lcSpotifyd) << "spotifyd not found on PATH — install it (see README) for automatic playback; "
                                 "MPRIS transport still works with any other player already running.";
        return;
    }
    if (!d->settings.getBool(QStringLiteral("spotifyAutoLaunch"), true)) {
        qCInfo(lcSpotifyd) << "auto-launch disabled";
        return;
    }
    if (spotifyPlayerAlreadyOnBus()) {
        qCInfo(lcSpotifyd) << "a spotify MPRIS player is already on the bus — not launching a second one.";
        return;
    }
    start();
}

SpotifydManager::~SpotifydManager()
{
    stop();
}

QString SpotifydManager::dataDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/polomodoro/spotifyd");
}

QString SpotifydManager::pidFilePath() const
{
    return dataDir() + QStringLiteral("/spotifyd.pid");
}

bool SpotifydManager::isRunning() const
{
    return d->process && d->process->state() != QProcess::NotRunning;
}

bool SpotifydManager::binaryFound() const
{
    return !d->binaryPath.isEmpty();
}

bool SpotifydManager::credentialsPresent() const
{
    return d->hasCredentials;
}

bool SpotifydManager::authenticating() const
{
    return d->authProcess && d->authProcess->state() != QProcess::NotRunning;
}

void SpotifydManager::refreshCredentialsPresent()
{
    const bool present = cacheHasCredentialFiles(dataDir());
    if (present == d->hasCredentials)
        return;
    d->hasCredentials = present;
    qCInfo(lcSpotifyd) << "credentials present:" << present;
    emit credentialsChanged();
}

void SpotifydManager::killStaleDaemon()
{
    QFile f(pidFilePath());
    if (!f.open(QIODevice::ReadOnly))
        return;
    const qint64 pid = QString::fromUtf8(f.readAll().trimmed()).toLongLong();
    f.close();
    if (pid <= 0 || pid == ::getpid()) {
        QFile::remove(pidFilePath());
        return;
    }

    QFile cmd(QStringLiteral("/proc/%1/cmdline").arg(pid));
    if (!cmd.open(QIODevice::ReadOnly)) {
        qCInfo(lcSpotifyd) << "stale pidfile" << pid << "— process gone";
        QFile::remove(pidFilePath());
        return;
    }
    const QByteArray cmdline = cmd.readAll();
    if (!cmdline.contains("spotifyd")) {
        qCInfo(lcSpotifyd) << "pidfile" << pid << "is not spotifyd — removing";
        QFile::remove(pidFilePath());
        return;
    }

    qCInfo(lcSpotifyd) << "killing leftover spotifyd pid" << pid;
    ::kill(static_cast<pid_t>(pid), SIGTERM);
    for (int i = 0; i < 20 && QFile::exists(QStringLiteral("/proc/%1").arg(pid)); ++i)
        QThread::msleep(50);
    if (QFile::exists(QStringLiteral("/proc/%1").arg(pid))) {
        qCWarning(lcSpotifyd) << "leftover still alive, sending SIGKILL to" << pid;
        ::kill(static_cast<pid_t>(pid), SIGKILL);
        QThread::msleep(50);
    }
    QFile::remove(pidFilePath());
}

void SpotifydManager::start()
{
    if (isRunning() || !binaryFound())
        return;
    if (authenticating()) {
        qCInfo(lcSpotifyd) << "deferring start — authenticate is still running";
        return;
    }

    killStaleDaemon();
    QDir().mkpath(dataDir());

    QString deviceName = d->settings.getString(QStringLiteral("spotifyDeviceName")).trimmed();
    if (deviceName.isEmpty())
        deviceName = QStringLiteral("Polomodoro");

    refreshCredentialsPresent();

    QStringList args{
        QStringLiteral("--no-daemon"),
        QStringLiteral("--cache-path"), dataDir(),
        QStringLiteral("--device-name"), deviceName,
        QStringLiteral("--dbus-type"), QStringLiteral("session"),
        QStringLiteral("--use-mpris=true"),
        QStringLiteral("--pid"), pidFilePath(),
        QStringLiteral("--device-type"), QStringLiteral("computer"),
    };
    if (qEnvironmentVariableIsSet("POLOMODORO_VERBOSE"))
        args << QStringLiteral("-v");

    qCInfo(lcSpotifyd) << "starting" << d->binaryPath << args
                       << "credentials:" << d->hasCredentials;

    d->stopping = false;
    d->process = std::make_unique<QProcess>();
    d->process->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(d->process.get(), &QProcess::stateChanged, this, [this](QProcess::ProcessState state) {
        qCInfo(lcSpotifyd) << "process state" << state << "pid"
                           << (d->process ? d->process->processId() : 0);
        emit runningChanged();
    });
    connect(d->process.get(), &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if (d->stopping) {
            qCInfo(lcSpotifyd) << "stop: QProcess reports" << err
                               << "(SIGTERM is expected; Unix maps it to Crashed)";
            return;
        }
        qCWarning(lcSpotifyd) << "process error:" << err << d->process->errorString();
    });
    connect(d->process.get(), &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
                const bool intentional = d->stopping;
                qCInfo(lcSpotifyd) << "exited code" << exitCode << "status" << status
                                   << (intentional ? "(stopped by us)" : "(unexpected)");
                if (!intentional && (status != QProcess::NormalExit || exitCode != 0))
                    qCWarning(lcSpotifyd) << "spotifyd exited abnormally (code" << exitCode << ")";
                QFile::remove(pidFilePath());
            });

    d->process->start(d->binaryPath, args);
    if (!d->process->waitForStarted(4000)) {
        qCWarning(lcSpotifyd) << "failed to start:" << d->process->errorString();
        d->process.reset();
        emit runningChanged();
        return;
    }
    qCInfo(lcSpotifyd) << "running pid" << d->process->processId();
    if (!d->hasCredentials) {
        qCWarning(lcSpotifyd) << "no librespot credentials in" << dataDir()
                              << "— daemon is Zeroconf-only and the Web API cannot play here until "
                                 "Settings → Music → Sign in to spotifyd completes (this is a different "
                                 "login from the playlist/search account).";
    }
}

void SpotifydManager::stop()
{
    if (d->authProcess && d->authProcess->state() != QProcess::NotRunning) {
        d->authProcess->terminate();
        d->authProcess->waitForFinished(2000);
    }
    d->authProcess.reset();

    if (!d->process)
        return;
    d->stopping = true;
    if (d->process->state() != QProcess::NotRunning) {
        const qint64 pid = d->process->processId();
        qCInfo(lcSpotifyd) << "stopping pid" << pid;
        d->process->terminate();
        if (!d->process->waitForFinished(3000)) {
            qCWarning(lcSpotifyd) << "did not exit after SIGTERM, killing" << pid;
            d->process->kill();
            d->process->waitForFinished(1000);
        }
    }
    d->process.reset();
    QFile::remove(pidFilePath());
}

void SpotifydManager::restart()
{
    qCInfo(lcSpotifyd) << "restart requested";
    stop();
    start();
}

void SpotifydManager::authenticate()
{
    if (!binaryFound()) {
        qCWarning(lcSpotifyd) << "authenticate: binary not found";
        return;
    }
    if (authenticating()) {
        qCInfo(lcSpotifyd) << "authenticate already in progress";
        return;
    }

    qCInfo(lcSpotifyd) << "starting spotifyd authenticate on 127.0.0.1:" << kAuthRedirectPort;
    stop();

    QDir().mkpath(dataDir());
    d->authProcess = std::make_unique<QProcess>();
    d->authProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(d->authProcess.get(), &QProcess::stateChanged, this, [this](QProcess::ProcessState) {
        emit authenticatingChanged();
    });
    connect(d->authProcess.get(), &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
                qCInfo(lcSpotifyd) << "authenticate finished code" << exitCode << "status" << status;
                refreshCredentialsPresent();
                d->authProcess.reset();
                emit authenticatingChanged();
                if (d->hasCredentials) {
                    if (d->settings.getBool(QStringLiteral("spotifyAutoLaunch"), true))
                        QTimer::singleShot(200, this, [this]() { start(); });
                } else {
                    qCWarning(lcSpotifyd) << "authenticate exited without writing credentials";
                    if (d->settings.getBool(QStringLiteral("spotifyAutoLaunch"), true))
                        QTimer::singleShot(200, this, [this]() { start(); });
                }
            });

    const QStringList args{
        QStringLiteral("authenticate"),
        QStringLiteral("--cache-path"), dataDir(),
        QStringLiteral("--oauth-port"), QString::number(kAuthRedirectPort),
    };
    d->authProcess->start(d->binaryPath, args);
    emit authenticatingChanged();
    if (!d->authProcess->waitForStarted(4000)) {
        qCWarning(lcSpotifyd) << "authenticate failed to start:" << d->authProcess->errorString();
        d->authProcess.reset();
        emit authenticatingChanged();
        start();
    }
}

} // namespace polomodoro
