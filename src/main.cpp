#include <polomodoro/BackgroundController.h>
#include <polomodoro/BackgroundManager.h>
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
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDateTime>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>
#include <qqml.h>

#include <polomodoro/TaskTreeModel.h>
#include <QtWebEngineQuick/QQuickWebEngineProfile>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

int main(int argc, char *argv[])
{
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
