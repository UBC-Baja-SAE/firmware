#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "core/cansocket.h"

int main(int argc, char *argv[]) {

    //Launch Application
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load("qrc:/qt/qml/app/src/ui/main.qml");

    CanSocket canSocket;
    canSocket.connectDevice();

    return app.exec();
}