#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThread>
#include "core/cansocket.h"
#include "core/dbcparser.h"
#include "core/foxglove.h"

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

    QThread* canThread = new QThread();
    QThread* parserThread = new QThread();
    QThread* foxgloveThread = new QThread();

    CanSocket* canSocket = new CanSocket();
    DbcParser* dbcParser = new DbcParser();
    FoxgloveSink* foxgloveSink = new FoxgloveSink();

    canSocket->moveToThread(canThread);
    dbcParser->moveToThread(parserThread);
    foxgloveSink->moveToThread(foxgloveThread);

    QObject::connect(canThread, &QThread::started, canSocket, &CanSocket::connectDevice);

    QObject::connect(foxgloveSink, &FoxgloveSink::serverStarted, dbcParser, [dbcParser]() {
        dbcParser->loadDbcFiles({":/test.dbc"});
    });
   QObject::connect(foxgloveThread, &QThread::started, foxgloveSink, [foxgloveSink]() {
        foxgloveSink->startServer(8765);
        foxgloveSink->startMcapRecording("/Users/bfrzn/git/firmware/projects/minimaldash/logs");
    });

    QObject::connect(canSocket, &CanSocket::rawFrameReceived,
                     dbcParser, &DbcParser::processFrame);

    QObject::connect(dbcParser, &DbcParser::foxgloveTopicsGenerated,
                     foxgloveSink, &FoxgloveSink::registerTopics);

    QObject::connect(dbcParser, &DbcParser::frameParsed,
                     foxgloveSink, &FoxgloveSink::broadcastPayload);

    foxgloveThread->start();
    parserThread->start();
    canThread->start();

    int exitCode = app.exec();

    canThread->quit();
    parserThread->quit();
    foxgloveThread->quit();

    canThread->wait();
    parserThread->wait();
    foxgloveThread->wait();

    delete canSocket;
    delete dbcParser;
    delete foxgloveSink;

    delete canThread;
    delete parserThread;
    delete foxgloveThread;

    return exitCode;
}