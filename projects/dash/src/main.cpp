#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCanBusDevice>
#include <QQuickStyle>
#include <QDir>
#include <QUrl>
#include "src/core/foxglove.h"

//Fallback for music path
#ifndef PROJECT_SOURCE_DIR
#define PROJECT_SOURCE_DIR "."
#endif

//Backend logic
#ifdef RELEASE
#include "src/core/can_socket.h"
using CanBackend = CanSocket;
#else
#include "src/core/debug_socket.h"
using CanBackend = DebugSocket;
#endif

int main(int argc, char *argv[]) {
//EGLFS backend when running on linux
#ifdef RELEASE
    qputenv("QT_QPA_PLATFORM", "eglfs");
    qputenv("QT_QPA_EGLFS_INTEGRATION", "eglfs_kms");
    qputenv("QT_QPA_EGLFS_KMS_CONFIG", ":/eglfs.json");
#endif

    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");

    CanBackend canAdapter;
    canAdapter.connectToDevice("can0");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("CanAdapter", &canAdapter);

    FoxgloveServer telemetryServer;
    telemetryServer.start();

    QObject::connect(&canAdapter, &CanBackend::foxglovePayloadReady,
                     &telemetryServer, &FoxgloveServer::broadcastCanFrame);

    //Finds music in source folder
    QString musicPath = QDir(PROJECT_SOURCE_DIR).filePath("src/ui/assets/music");
    QString musicUrl = QUrl::fromLocalFile(musicPath).toString();
    engine.rootContext()->setContextProperty("musicFolderUrl", musicUrl);

    using namespace Qt::StringLiterals;
    const QUrl url(u"qrc:/qt/qml/Roxy/src/ui/main.qml"_s);
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}