#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDir>
#include <QUrl>
#include "src/core/foxglove.h"
#include "src/core/controls.h"
#include "src/core/can_socket.h"
#include "src/core/webcam.h"

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

    Webcam dashCam;
    dashCam.start();
    QObject::connect(&dashCam, &Webcam::frameReady, &telemetryServer, &FoxgloveServer::broadcastImage);

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


    // Find the dashcam UI component by its objectName and link it to the C++ camera
    QObject* videoOut = engine.rootObjects().first()->findChild<QObject*>("dashVideoOutput");
    if (videoOut) {
        dashCam.setQmlVideoOutput(videoOut);
    }

    return app.exec();
}