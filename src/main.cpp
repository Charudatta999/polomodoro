#include <polomodoro/BackgroundController.h>
#include <polomodoro/BackgroundManager.h>
#include <polomodoro/DatabaseManager.h>
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

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDateTime>
#include <QQmlContext>
#include <QQuickStyle>
#include <qqml.h>

#include <polomodoro/TaskTreeModel.h>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName(QStringLiteral("Polomodoro"));
    QCoreApplication::setApplicationName(QStringLiteral("polomodoro"));
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

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
    engine.rootContext()->setContextProperty(QStringLiteral("timerController"), &timerController);
    engine.rootContext()->setContextProperty(QStringLiteral("taskController"), &taskController);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsController"), &settingsController);
    engine.rootContext()->setContextProperty(QStringLiteral("backgroundController"), &backgroundController);
    engine.rootContext()->setContextProperty(QStringLiteral("spotifyController"), &spotifyController);
    engine.rootContext()->setContextProperty(QStringLiteral("spotifyBridge"), &spotifyBridge);
    engine.rootContext()->setContextProperty(QStringLiteral("windowLayout"), &windowLayout);

    qmlRegisterUncreatableType<polomodoro::TaskTreeModel>(
        "Polomodoro", 1, 0, "TaskTreeModel", QStringLiteral("Use taskController.model"));

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

    return app.exec();
}
