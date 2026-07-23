#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QQmlContext>
#include "core/cansocket.h"
#include "core/dbcparser.h"
#include "core/foxglove.h"
#include "core/dash.h"
#include "peripherals/webcam.h"
#include <QLoggingCategory>

int main(int argc, char *argv[]) {

#ifdef LINUX
    qputenv("QT_QPA_PLATFORM", "eglfs");
    qputenv("QT_QPA_EGLFS_HIDECURSOR", "1"); // Add this line

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
#endif

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // 1. Instantiate the Webcam backend
    Webcam webcamBackend;
    engine.rootContext()->setContextProperty("WebcamBackend", &webcamBackend);

#ifdef ENV_RELEASE
    engine.rootContext()->setContextProperty("IsReleaseBuild", true);
    bool enableMcap = true;

    // 2. Start hardware capture for release builds
    webcamBackend.start();
#else
    engine.rootContext()->setContextProperty("IsReleaseBuild", false);
    bool enableMcap = false;
#endif

    bool enableWebsocket = true;

    Dash dashBackend;
    engine.rootContext()->setContextProperty("Data", &dashBackend);

    // --- NEW: Instantiate the objects BEFORE loading QML ---
    CanSocket* canSocket = new CanSocket();
    DbcParser* dbcParser = new DbcParser();
    FoxgloveSink* foxgloveSink = new FoxgloveSink();

    // --- NEW: Expose the parser to QML ---
    engine.rootContext()->setContextProperty("DbcParser", dbcParser);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    // Now load the QML
    engine.load("qrc:/qt/qml/app/src/ui/main.qml");

    QThread* canThread = new QThread();
    QThread* parserThread = new QThread();
    QThread* foxgloveThread = new QThread();

    canSocket->moveToThread(canThread);
    dbcParser->moveToThread(parserThread);
    foxgloveSink->moveToThread(foxgloveThread);

    QObject::connect(canThread, &QThread::started, canSocket, [canSocket]() {
        canSocket->connectDevice();
    });

    QObject::connect(parserThread, &QThread::started, dbcParser, [dbcParser]() {
        dbcParser->loadDbcFiles({":/mochi.dbc"});
    });

    QObject::connect(foxgloveThread, &QThread::started, foxgloveSink, [foxgloveSink, enableWebsocket, enableMcap]() {
        if (enableWebsocket) {
            foxgloveSink->startServer(8765);
        }
        if (enableMcap) {
#ifdef ENV_RELEASE
        QString logDir = QStringLiteral("/home/ubcbaja/firmware/logs");
#else
        QString logDir = QCoreApplication::applicationDirPath() + "/logs";
#endif
        foxgloveSink->startMcapRecording(logDir);
            foxgloveSink->startMcapRecording(logDir);
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

    // 3. Connect the Webcam signals to FoxgloveSink slots
    // Note: Adjust the slots (&FoxgloveSink::broadcastImage / broadcastAudio)
    // to match whatever you named the receiving functions in your foxglove.h wrapper.
    QObject::connect(&webcamBackend, &Webcam::frameReady,
                     foxgloveSink, &FoxgloveSink::broadcastImage);

    QObject::connect(&webcamBackend, &Webcam::audioReady,
                     foxgloveSink, &FoxgloveSink::broadcastAudio);

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