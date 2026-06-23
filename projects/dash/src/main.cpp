#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDir>
#include <QUrl>
#include "src/core/foxglove.h"
#include "src/core/controls.h"
#include "src/core/can_socket.h"

//Fallback for music path
#ifndef PROJECT_SOURCE_DIR
#define PROJECT_SOURCE_DIR "."
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

    CanSocket canAdapter;
    canAdapter.connectToDevice("can0");

    Controls wiimote;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("CanAdapter", &canAdapter);

    FoxgloveServer telemetryServer;
    telemetryServer.start();

    QObject::connect(&canAdapter, &CanSocket::foxglovePayloadReady,
                     &telemetryServer, &FoxgloveServer::broadcastCanFrame);

    QObject::connect(&wiimote, &Controls::foxglovePayloadReady,
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