#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QFile>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    using namespace Qt::StringLiterals;
    const QUrl url(u"qrc:/qt/qml/app/src/ui/main.qml"_s);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}