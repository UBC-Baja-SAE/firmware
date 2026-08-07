#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QQmlContext>
#include <QProcess>
#include <QTimer>
#include "core/cansocket.h"
#include "core/dbcparser.h"
#include "core/foxglove.h"
#include "core/dash.h"
#include "peripherals/webcam.h"
#include <QLoggingCategory>

int main(int argc, char *argv[]) {

#ifdef LINUX
    qputenv("QT_QPA_PLATFORM", "eglfs");
    qputenv("QT_QPA_EGLFS_HIDECURSOR", "1");

    QString tempKmsPath = "/tmp/eglfs.json";
    QFile::remove(tempKmsPath);

    if (QFile::copy(":/qt/qml/app/assets/eglfs/eglfs.json", tempKmsPath)) {
        qputenv("QT_QPA_EGLFS_KMS_CONFIG", tempKmsPath.toLocal8Bit());
    } else {
        qCritical() << "Failed to extract EGLFS KMS config to /tmp";
    }

    QString tempDbcPath = "/tmp/mochi.dbc";
    QFile::remove(tempDbcPath);

    if (!QFile::copy(":/mochi.dbc", tempDbcPath)) {
        qCritical() << "Failed to extract dbc to /tmp";
    }

    QString tempWavPath = "/tmp/win95.wav";
    QFile::remove(tempWavPath);

    if (!QFile::copy(":/qt/qml/app/assets/sounds/win95.wav", tempWavPath)) {
        qCritical() << "Failed to extract win95.wav to /tmp";
    }
#endif

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    Webcam webcamBackend;
    engine.rootContext()->setContextProperty("WebcamBackend", &webcamBackend);

#ifdef ENV_RELEASE
    engine.rootContext()->setContextProperty("IsReleaseBuild", true);
    webcamBackend.start();

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, []() {
        // Increased from 400ms to 1500ms to guarantee the amplifier IC has time
        // to sync to the I2S clocks after a cold power-on.
        QTimer::singleShot(1500, []() {
            // Wrap the command in a shell to redirect ALL terminal output (errors and successes) to a log file
            QProcess::startDetached("/bin/sh", {"-c", "/usr/bin/aplay -D plughw:0,0 /tmp/win95.wav > /tmp/audio_debug.log 2>&1"});
        });
    });
#else
    engine.rootContext()->setContextProperty("IsReleaseBuild", false);
#endif

    bool enableWebsocket = true;

    Dash dashBackend;
    engine.rootContext()->setContextProperty("Data", &dashBackend);

    CanSocket* canSocket = new CanSocket();
    DbcParser* dbcParser = new DbcParser();
    FoxgloveSink* foxgloveSink = new FoxgloveSink();

    QThread* canThread = new QThread();
    QThread* parserThread = new QThread();
    QThread* foxgloveThread = new QThread();

    canSocket->moveToThread(canThread);
    dbcParser->moveToThread(parserThread);
    foxgloveSink->moveToThread(foxgloveThread);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load("qrc:/qt/qml/app/src/ui/main.qml");

    QObject::connect(canThread, &QThread::started, canSocket, [canSocket]() {
        canSocket->connectDevice();
    });

    QObject::connect(parserThread, &QThread::started, dbcParser, [dbcParser]() {
        dbcParser->loadDbcFiles({":/mochi.dbc"});
    });

    // Auto-start Websocket, but leave MCAP off until the toggle switch flips
    QObject::connect(foxgloveThread, &QThread::started, foxgloveSink, [foxgloveSink, enableWebsocket]() {
        if (enableWebsocket) {
            foxgloveSink->startServer(8765);
        }
    });

    QObject::connect(canSocket, &CanSocket::rawFrameReceived,
                     dbcParser, &DbcParser::processFrame);

    QObject::connect(dbcParser, &DbcParser::foxgloveTopicsGenerated,
                     foxgloveSink, &FoxgloveSink::registerTopics);

    QObject::connect(dbcParser, &DbcParser::frameParsed,
                     foxgloveSink, &FoxgloveSink::broadcastPayload);

    QObject::connect(dbcParser, &DbcParser::frameParsed,
                     &dashBackend, &Dash::onFrameParsed);

    QObject::connect(&dashBackend, &Dash::outboundFrameReady,
                     canSocket, &CanSocket::sendFrame,
                     Qt::QueuedConnection);

    // NEW: Route the toggle switch signal to FoxgloveSink across threads
    QObject::connect(&dashBackend, &Dash::loggingCommanded,
                     foxgloveSink, &FoxgloveSink::toggleLogging,
                     Qt::QueuedConnection);

    QObject::connect(&webcamBackend, &Webcam::frameReady,
                     foxgloveSink, &FoxgloveSink::broadcastImage,
                     Qt::QueuedConnection);

    QObject::connect(&webcamBackend, &Webcam::audioReady,
                     foxgloveSink, &FoxgloveSink::broadcastAudio,
                     Qt::QueuedConnection);

    QLoggingCategory::setFilterRules("qt.gui.imageio.jpeg.warning=false");

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