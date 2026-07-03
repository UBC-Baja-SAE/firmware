#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThread>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QQmlContext>
#include "core/settings.h"
#include "core/cansocket.h"
#include "core/dbcparser.h"
#include "core/foxglove.h"

int main(int argc, char *argv[]) {

    QGuiApplication app(argc, argv);

    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);

    bool enableWebsocket = settings.value("Foxglove/EnableWebsocket", true).toBool();
    bool enableMcap = settings.value("Foxglove/EnableMcap", false).toBool();
    int canBaudRate = settings.value("CAN/BaudRate", 500000).toInt();

    if (!QFile::exists(configPath)) {
        settings.setValue("Foxglove/EnableWebsocket", enableWebsocket);
        settings.setValue("Foxglove/EnableMcap", enableMcap);
        settings.setValue("CAN/BaudRate", canBaudRate);
        settings.sync();
        qInfo() << "Created default configuration file at:" << configPath;
    }

    QQmlApplicationEngine engine;

    AppSettings* appSettings = new AppSettings();
    engine.rootContext()->setContextProperty("AppSettings", appSettings);

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

    QObject::connect(canThread, &QThread::started, canSocket, [canSocket, canBaudRate]() {
        canSocket->connectDevice(canBaudRate);
    });

    QObject::connect(parserThread, &QThread::started, dbcParser, [dbcParser]() {
        dbcParser->loadDbcFiles({":/test.dbc"});
    });

    QObject::connect(foxgloveThread, &QThread::started, foxgloveSink, [foxgloveSink, enableWebsocket, enableMcap]() {
        if (enableWebsocket) {
            foxgloveSink->startServer(8765);
        }
        if (enableMcap) {
            QString logDir = QCoreApplication::applicationDirPath() + "/logs";
            foxgloveSink->startMcapRecording(logDir);
        }
    });

    QObject::connect(canSocket, &CanSocket::rawFrameReceived,
                     dbcParser, &DbcParser::processFrame);

    QObject::connect(dbcParser, &DbcParser::foxgloveTopicsGenerated,
                     foxgloveSink, &FoxgloveSink::registerTopics);

    QObject::connect(dbcParser, &DbcParser::frameParsed,
                     foxgloveSink, &FoxgloveSink::broadcastPayload);

    QObject::connect(appSettings, &AppSettings::websocketEnabledChanged,
                     foxgloveSink, &FoxgloveSink::toggleServer);

    QObject::connect(appSettings, &AppSettings::mcapEnabledChanged,
                     foxgloveSink, &FoxgloveSink::toggleLogging);

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