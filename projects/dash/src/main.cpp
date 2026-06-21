#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCanBusDevice>
#include <QQuickStyle>
#include <QDir>
#include <QUrl>
#include "src/core/can_socket.h"

// Safety fallback for music path
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

    // Forces the un-opinionated style for UI customization
    QQuickStyle::setStyle("Basic");

    CanSocket canAdapter;

    // Automatically connect on startup
    canAdapter.connectToDevice("can0");

    QQmlApplicationEngine engine;

    // Pass the CAN Adapter to QML
    engine.rootContext()->setContextProperty("CanAdapter", &canAdapter);

    // --- MUSIC PATH LOGIC ---
    // Tells the app exactly where the source code folder is
    QString musicPath = QDir(PROJECT_SOURCE_DIR).filePath("src/ui/assets/music");
    QString musicUrl = QUrl::fromLocalFile(musicPath).toString();
    engine.rootContext()->setContextProperty("musicFolderUrl", musicUrl);
    // ------------------------

    using namespace Qt::StringLiterals;
    const QUrl url(u"qrc:/qt/qml/Roxy/src/ui/main.qml"_s);
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}