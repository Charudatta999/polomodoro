#include <polomodoro/BackgroundController.h>
#include <polomodoro/BackgroundManager.h>
#include <polomodoro/DayTimelineModel.h>
#include <polomodoro/DatabaseManager.h>
#include <polomodoro/NotificationManager.h>
#include <polomodoro/SettingsController.h>
#include <polomodoro/SettingsStore.h>
#include <polomodoro/ShutdownGuard.h>
#include <polomodoro/SpotifyArtBridge.h>
#include <polomodoro/SpotifyController.h>
#include <polomodoro/TaskController.h>
#include <polomodoro/TaskTree.h>
#include <polomodoro/TimerController.h>
#include <polomodoro/TimerEngine.h>
#include <polomodoro/WindowLayoutManager.h>

#include <QDir>
#include <QFileInfo>
#include <QVersionNumber>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDateTime>
#include <QQmlContext>
#include <QQmlError>
#include <QLoggingCategory>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>
#include <qqml.h>

#include <polomodoro/TaskTreeModel.h>
#include <QtWebEngineQuick/QQuickWebEngineProfile>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

// This Qt build filters the "qml" logging category to nothing, so console.*
// and qmlWarning() (including delegate-creation failures, which are otherwise
// completely silent) never reach the terminal. POLOMODORO_VERBOSE=1 forces the
// rules open and installs a handler that prints everything.
static void verboseMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    static const char *names[] = {"DEBUG", "WARN", "CRIT", "FATAL", "INFO"};
    fprintf(stderr, "QT[%s] %s (%s:%d)\n", names[type <= 4 ? type : 0], qUtf8Printable(msg),
            ctx.file ? ctx.file : "-", ctx.line);
}

// Spotify's web player is DRM-gated and refuses to play without a Widevine
// CDM, reporting it as "you block protected content / incompatible browser".
// QtWebEngine only looks in ~/.config/chromium and ~/.config/google-chrome,
// which a machine with neither installed will not have — even though other
// Chromium-based apps here have already downloaded a perfectly good CDM. Find
// one and point QtWebEngine at it explicitly.
static QString findWidevineCdm()
{
    const QString home = QDir::homePath();
    const QStringList roots = {
        home + QStringLiteral("/.config/chromium/WidevineCdm"),
        home + QStringLiteral("/.config/google-chrome/WidevineCdm"),
        home + QStringLiteral("/.config/BraveSoftware/Brave-Browser/WidevineCdm"),
        home + QStringLiteral("/.config/Netflix/WidevineCdm"),
        home + QStringLiteral("/.cache/spotify/WidevineCdm"),
        QStringLiteral("/opt/google/chrome/WidevineCdm"),
        QStringLiteral("/usr/lib/chromium/WidevineCdm"),
    };

    QString best;
    QVersionNumber bestVersion;
    for (const QString &root : roots) {
        QDir dir(root);
        if (!dir.exists())
            continue;
        const QStringList versions =
            dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString &v : versions) {
            const QString so = root + QLatin1Char('/') + v
                + QStringLiteral("/_platform_specific/linux_x64/libwidevinecdm.so");
            if (!QFileInfo::exists(so))
                continue;
            const QVersionNumber version = QVersionNumber::fromString(v);
            if (best.isEmpty() || version > bestVersion) {
                best = so;
                bestVersion = version;
            }
        }
    }
    return best;
}

static QString g_widevineCdmPath;

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsSet("POLOMODORO_VERBOSE")) {
        QLoggingCategory::setFilterRules(QStringLiteral("*=true"));
        qInstallMessageHandler(verboseMessageHandler);
    }

    // Must happen before QtWebEngineQuick::initialize(): the flags are read
    // when Chromium starts up, not when a view is created.
    g_widevineCdmPath = findWidevineCdm();
    if (const QString cdm = g_widevineCdmPath; !cdm.isEmpty()) {
        QString flags = qEnvironmentVariable("QTWEBENGINE_CHROMIUM_FLAGS");
        if (!flags.contains(QStringLiteral("--widevine-path"))) {
            if (!flags.isEmpty())
                flags += QLatin1Char(' ');
            flags += QStringLiteral("--widevine-path=") + cdm;
            qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags.toUtf8());
        }
    } else {
        fprintf(stderr, "Widevine CDM not found — Spotify playback will be refused.\n");
    }

    QCoreApplication::setOrganizationName(QStringLiteral("Polomodoro"));
    QCoreApplication::setApplicationName(QStringLiteral("polomodoro"));
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    const int geistSansId = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Geist-Regular.ttf"));
    const int geistMonoId = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/GeistMono-Regular.ttf"));
    if (geistSansId >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(geistSansId);
        if (!families.isEmpty())
            app.setFont(QFont(families.first()));
    }
    if (geistMonoId >= 0) {
        Q_UNUSED(QFontDatabase::applicationFontFamilies(geistMonoId));
    }

    polomodoro::DatabaseManager database;
    if (!database.open())
        return 1;

    polomodoro::SettingsStore settings(database);
    settings.seedDefaults();

    polomodoro::TimerEngine timerEngine;
    polomodoro::TaskTree taskTree(database);
    taskTree.load();

    polomodoro::BackgroundManager backgroundManager(settings);
    polomodoro::SpotifyArtBridge spotifyBridge;

    polomodoro::TimerController timerController(timerEngine, settings);
    polomodoro::TaskController taskController(taskTree, settings);
    polomodoro::SettingsController settingsController(settings);
    polomodoro::BackgroundController backgroundController(backgroundManager, settings);
    polomodoro::SpotifyController spotifyController(spotifyBridge);
    polomodoro::WindowLayoutManager windowLayout(settings);

    polomodoro::DayTimelineModel dayTimeline(taskTree);
    // Any task mutation can add, move or remove a block on the timeline.
    QObject::connect(&taskController, &polomodoro::TaskController::tasksChanged, &dayTimeline,
                     &polomodoro::DayTimelineModel::refresh);
    // Per spec the now line and the in-progress block share one 60 s timer.
    QTimer nowTicker;
    QObject::connect(&nowTicker, &QTimer::timeout, &dayTimeline, &polomodoro::DayTimelineModel::tick);
    nowTicker.start(60000);

    polomodoro::ShutdownGuard shutdownGuard(database, taskTree);
    shutdownGuard.startHeartbeat(10000);

    polomodoro::NotificationManager notificationManager;
    QObject::connect(&taskController, &polomodoro::TaskController::targetReached, &app,
                     [&](const QString &, const QString &title) {
                         notificationManager.notify(QStringLiteral("Target reached"),
                                                     title + QStringLiteral(" hit its time target."));
                     });
    QObject::connect(&timerController, &polomodoro::TimerController::workSegmentCompleted, &app,
                     [&](qint64 durationMs) {
                         Q_UNUSED(durationMs);
                         notificationManager.notify(QStringLiteral("Pomodoro complete"),
                                                     QStringLiteral("Time for a break."));
                     });

    const QString spotifyData =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/spotify-profile");
    const QString spotifyCache =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QStringLiteral("/spotify-profile");
    QDir().mkpath(spotifyData);
    QDir().mkpath(spotifyCache);

    QQuickWebEngineProfile spotifyProfile(QStringLiteral("polomodoro-spotify"));
    spotifyProfile.setOffTheRecord(false);
    spotifyProfile.setPersistentStoragePath(spotifyData);
    spotifyProfile.setCachePath(spotifyCache);
    spotifyProfile.setPersistentCookiesPolicy(QQuickWebEngineProfile::ForcePersistentCookies);
    spotifyProfile.setHttpCacheType(QQuickWebEngineProfile::DiskHttpCache);
    spotifyProfile.setPersistentPermissionsPolicy(
        QQuickWebEngineProfile::PersistentPermissionsPolicy::StoreOnDisk);
    spotifyProfile.setHttpUserAgent(
        QStringLiteral("Mozilla/5.0 (Linux; Android 14; Pixel 8) AppleWebKit/537.36 "
                       "(KHTML, like Gecko) Chrome/122.0.0.0 Mobile Safari/537.36"));

    QObject::connect(&timerController, &polomodoro::TimerController::workSegmentCompleted, &app,
                     [&](qint64 durationMs) {
                         const QDateTime started = QDateTime::currentDateTimeUtc().addMSecs(-durationMs);
                         for (const polomodoro::TaskNode *task : taskTree.activeTasks())
                             taskTree.addSession(task->id, started, durationMs, QStringLiteral("pomodoro"));
                     });

    QObject::connect(&spotifyBridge, &polomodoro::SpotifyArtBridge::metadataChanged, &backgroundController, [&]() {
        backgroundManager.setSpotifyArtUrl(spotifyBridge.artUrl());
        emit backgroundController.backgroundChanged();
    });

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &engine, [](const QList<QQmlError> &warnings) {
        for (const auto &w : warnings)
            fprintf(stderr, "QML WARNING: %s\n", qUtf8Printable(w.toString()));
    });
    auto *ctx = engine.rootContext();
    ctx->setContextProperty(QStringLiteral("TimerController"), &timerController);
    ctx->setContextProperty(QStringLiteral("TaskController"), &taskController);
    ctx->setContextProperty(QStringLiteral("SettingsController"), &settingsController);
    ctx->setContextProperty(QStringLiteral("BackgroundController"), &backgroundController);
    ctx->setContextProperty(QStringLiteral("SpotifyController"), &spotifyController);
    ctx->setContextProperty(QStringLiteral("WindowLayoutManager"), &windowLayout);
    ctx->setContextProperty(QStringLiteral("DayTimelineModel"), &dayTimeline);
    // Empty when no CDM was found; the Spotify drawer shows the DRM notice.
    ctx->setContextProperty(QStringLiteral("WidevineCdmPath"), g_widevineCdmPath);
    ctx->setContextProperty(QStringLiteral("timerController"), &timerController);
    ctx->setContextProperty(QStringLiteral("taskController"), &taskController);
    ctx->setContextProperty(QStringLiteral("settingsController"), &settingsController);
    ctx->setContextProperty(QStringLiteral("backgroundController"), &backgroundController);
    ctx->setContextProperty(QStringLiteral("spotifyController"), &spotifyController);
    ctx->setContextProperty(QStringLiteral("spotifyBridge"), &spotifyBridge);
    ctx->setContextProperty(QStringLiteral("windowLayout"), &windowLayout);
    ctx->setContextProperty(QStringLiteral("SpotifyWebProfile"), &spotifyProfile);

    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/qml/Theme.qml")), "Polomodoro", 1, 0, "Theme");
    qmlRegisterUncreatableType<polomodoro::TaskTreeModel>(
        "Polomodoro", 1, 0, "TaskTreeModel", QStringLiteral("Use TaskController.model"));

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);
    if (engine.rootObjects().isEmpty())
        return 1;

    // Debug affordance: grabWindow() (used by POLOMODORO_SCREENSHOT below)
    // captures only the client-rendered surface, so it cannot show whether the
    // compositor is drawing its own titlebar around it. This reports the real
    // window flags and frame margins instead — a nonzero top margin or
    // frame-vs-geometry mismatch means a system decoration is present despite
    // Qt.FramelessWindowHint being set, e.g. because a Wayland compositor
    // dropped the hint across a surface recreation.
    if (qEnvironmentVariableIsSet("POLOMODORO_FRAME_TEST")) {
        QTimer::singleShot(qEnvironmentVariableIntValue("POLOMODORO_FRAME_TEST"), &app, [&windowLayout, &engine]() {
            auto *w = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
            if (!w) { QCoreApplication::exit(2); return; }
            const QRect g = w->geometry();
            const QRect fg = w->frameGeometry();
            const QMargins m = w->frameMargins();
            fprintf(stderr,
                    "FRAME mode=%d frameless=%d geom=%dx%d min=%dx%d max=%dx%d "
                    "margins(t%d) decorated=%s\n",
                    windowLayout.viewMode(),
                    (w->flags() & Qt::FramelessWindowHint) ? 1 : 0,
                    g.width(), g.height(),
                    w->minimumWidth(), w->minimumHeight(),
                    w->maximumWidth(), w->maximumHeight(),
                    m.top(),
                    (m.top() > 0 || fg.height() > g.height()) ? "YES" : "no");
            QCoreApplication::exit(0);
        });
    }

    // Debug affordance: this desktop denies both the GNOME Shell and the xdg
    // screenshot portals, so the only way to see what the app actually renders
    // is to have it grab itself. POLOMODORO_VIEW_MODE forces a view mode first
    // so bar/PiP can be captured without driving the UI by hand.
    if (qEnvironmentVariableIsSet("POLOMODORO_SCREENSHOT")) {
        const QString shotPath = qEnvironmentVariable("POLOMODORO_SCREENSHOT");
        const int delayMs = qEnvironmentVariableIntValue("POLOMODORO_SCREENSHOT_DELAY") > 0
            ? qEnvironmentVariableIntValue("POLOMODORO_SCREENSHOT_DELAY")
            : 1500;
        // Switch modes from inside the running event loop, not before exec():
        // pre-exec the window is not yet mapped and the size constraints behave
        // differently, which is not what a user pressing Ctrl+Shift+2 hits.
        if (qEnvironmentVariableIsSet("POLOMODORO_VIEW_MODE")) {
            const int mode = qEnvironmentVariableIntValue("POLOMODORO_VIEW_MODE");
            QTimer::singleShot(delayMs / 2, &app, [&windowLayout, mode]() {
                windowLayout.setMode(mode);
            });
        }

        QTimer::singleShot(delayMs, &app, [&engine, shotPath]() {
            auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
            if (!window) {
                fprintf(stderr, "SCREENSHOT: root object is not a QQuickWindow\n");
                QCoreApplication::exit(2);
                return;
            }
            const QImage shot = window->grabWindow();
            const bool ok = !shot.isNull() && shot.save(shotPath);
            fprintf(stderr, "SCREENSHOT: %s -> %s (%dx%d)\n", ok ? "saved" : "FAILED",
                    qUtf8Printable(shotPath), shot.width(), shot.height());
            QCoreApplication::exit(ok ? 0 : 3);
        });
    }

    return app.exec();
}
