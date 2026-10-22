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

    auto *canSocket    = new CanSocket();
    auto *dbcParser    = new DbcParser();
    auto *foxgloveSink = new FoxgloveSink();
    auto *guiBridge    = new GuiBridge(&app);

    // On background thread
    canSocket->moveToThread(&canThread);
    dbcParser->moveToThread(&canThread);
    foxgloveSink->moveToThread(&canThread);

    QObject::connect(&canThread, &QThread::finished, canSocket,    &QObject::deleteLater);
    QObject::connect(&canThread, &QThread::finished, dbcParser,    &QObject::deleteLater);
    QObject::connect(&canThread, &QThread::finished, foxgloveSink, &QObject::deleteLater);

    // Connect to background thread
    QObject::connect(dbcParser, &DbcParser::topicDiscovered,
                     foxgloveSink, &FoxgloveSink::registerTopic);
    QObject::connect(canSocket, &CanSocket::framesReceived,
                     dbcParser, &DbcParser::processFrames);
    QObject::connect(dbcParser, &DbcParser::messageDecoded,
                     foxgloveSink, &FoxgloveSink::publishSignals);
    QObject::connect(dbcParser, &DbcParser::messageDecoded,
                     guiBridge, &GuiBridge::onDecoded);

    QObject::connect(&canThread, &QThread::started, foxgloveSink,
                     [foxgloveSink, dbcParser, canSocket]() {
        foxgloveSink->startServer(8765);
        //foxgloveSink->setMcapLogging(true, QStringLiteral("/home/ubcbaja/firmware/logs")); //Comment out for debugging/no logging

        if (!dbcParser->loadDbc(QStringLiteral(":/roxy.dbc"))) {
            qWarning() << "DBC load failed, not starting CAN";
            return;
        }

        if (!canSocket->start(QString(), QStringLiteral("can0")))
            qWarning() << "CAN start failed";
    });

    engine.rootContext()->setContextProperty("guiBridge", guiBridge);

    canThread.start();

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/app/src/ui/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        canThread.quit();
        canThread.wait();
        return -1;
    }

    const int ret = app.exec();

    canThread.quit();
    canThread.wait();

    return ret;
}