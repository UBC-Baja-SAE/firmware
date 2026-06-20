#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCanBusDevice>
#include "src/core/can_socket.h"

int main(int argc, char *argv[]) {
    //EGLFS backend when running on linux
    #ifdef RELEASE
        qputenv("QT_QPA_PLATFORM", "eglfs");
        qputenv("QT_QPA_EGLFS_INTEGRATION", "eglfs_kms");
        qputenv("QT_QPA_EGLFS_KMS_CONFIG", ":/eglfs.json");
    #endif

    QGuiApplication app(argc, argv);

    CanSocket canAdapter;

    // Automatically connect on startup
    canAdapter.connectToDevice("can0");

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("CanAdapter", &canAdapter);

    using namespace Qt::StringLiterals;
    const QUrl url(u"qrc:/qt/qml/Roxy/src/ui/main.qml"_s);
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
