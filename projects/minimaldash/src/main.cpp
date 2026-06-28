#include <QGuiApplication>
#include <QQmlApplicationEngine>

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

    return app.exec();
}