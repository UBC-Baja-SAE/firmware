#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QDebug>
#include "core/cansocket.h"
#include "core/dbcparser.h"
#include "core/foxglove.h"
#include "core/guibridge.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    QThread canThread;
    QThread foxgloveThread;

    auto *canSocket    = new CanSocket();
    auto *dbcParser    = new DbcParser();
    auto *foxgloveSink = new FoxgloveSink();
    auto *guiBridge    = new GuiBridge(&app);

    canSocket->moveToThread(&canThread);
    dbcParser->moveToThread(&canThread);
    foxgloveSink->moveToThread(&foxgloveThread);

    QObject::connect(&canThread,  &QThread::finished, canSocket,    &QObject::deleteLater);
    QObject::connect(&canThread,  &QThread::finished, dbcParser,    &QObject::deleteLater);
    QObject::connect(&foxgloveThread, &QThread::finished, foxgloveSink, &QObject::deleteLater);

    QObject::connect(canSocket, &CanSocket::framesReceived, dbcParser, &DbcParser::processFrames);
    QObject::connect(dbcParser, &DbcParser::sample, foxgloveSink, &FoxgloveSink::publishSample);
    QObject::connect(dbcParser, &DbcParser::sample, guiBridge, &GuiBridge::onSample, Qt::DirectConnection);

    QObject::connect(&foxgloveThread, &QThread::started, foxgloveSink, [foxgloveSink]() {
        foxgloveSink->startServer(8765);
        //foxgloveSink->startMcapLogging(true, QStringLiteral("/home/ubcbaja/firmware/logs")); //Comment out for debugging/no logging
    });

    QObject::connect(&canThread, &QThread::started, dbcParser, [dbcParser, canSocket]() {
        if (!dbcParser->loadDbc(QStringLiteral(":/roxy.dbc"))) {
            qWarning() << "DBC load failed, not starting CAN";
            return;
        }
        if (!canSocket->start(QString(), QStringLiteral("can0")))
            qWarning() << "CAN start failed";
    });

    engine.rootContext()->setContextProperty("guiBridge", guiBridge);

    foxgloveThread.start();
    canThread.start();

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/app/src/ui/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        canThread.quit();   canThread.wait();
        foxgloveThread.quit();  foxgloveThread.wait();
        return -1;
    }

    const int ret = app.exec();

    canThread.quit();   canThread.wait();
    foxgloveThread.quit();  foxgloveThread.wait();

    return ret;
}