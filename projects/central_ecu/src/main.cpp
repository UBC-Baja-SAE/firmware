#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QDebug>
#include "core/cansocket.h"
#include "core/dbcparser.h"
#include "core/foxglove.h"
#include "core/guibridge.h"
#ifdef Q_OS_LINUX
#include "peripherals/webcam.h"
#endif

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    QThread canThread;
    QThread foxgloveThread;
#ifdef Q_OS_LINUX
    QThread camThread;
#endif

    auto *canSocket = new CanSocket();
    auto *dbcParser = new DbcParser();
    auto *foxgloveSink = new FoxgloveSink();
    auto *guiBridge = new GuiBridge(&app);
#ifdef Q_OS_LINUX
    // TODO: replace /dev/video0 with the stable path from `ls -l /dev/v4l/by-path`
    QList<Webcam *> webcams;
    // Front
    webcams << new Webcam(QStringLiteral("front"),
                      QStringLiteral("/dev/v4l/by-id/usb-046d_0819_D813F890-video-index0"),
                      QStringLiteral("plughw:CARD=front,DEV=0"));
#endif

    canSocket->moveToThread(&canThread);
    dbcParser->moveToThread(&canThread);
    foxgloveSink->moveToThread(&foxgloveThread);
#ifdef Q_OS_LINUX
    for (auto *w : webcams) w->moveToThread(&camThread);
#endif

    QObject::connect(&canThread,  &QThread::finished, canSocket, &QObject::deleteLater);
    QObject::connect(&canThread,  &QThread::finished, dbcParser, &QObject::deleteLater);
    QObject::connect(&foxgloveThread, &QThread::finished, foxgloveSink, &QObject::deleteLater);
#ifdef Q_OS_LINUX
    for (auto *w : webcams) QObject::connect(&camThread, &QThread::finished, w, &QObject::deleteLater);
#endif

    QObject::connect(canSocket, &CanSocket::framesReceived, dbcParser, &DbcParser::processFrames);
    QObject::connect(dbcParser, &DbcParser::sample, foxgloveSink, &FoxgloveSink::publishSample);
    QObject::connect(dbcParser, &DbcParser::sample, guiBridge, &GuiBridge::onSample, Qt::DirectConnection);
#ifdef Q_OS_LINUX
    for (auto *w : webcams) {
        QObject::connect(w, &Webcam::frameReady, foxgloveSink, &FoxgloveSink::publishImage);
        QObject::connect(w, &Webcam::audioReady, foxgloveSink, &FoxgloveSink::publishAudio);
    }
#endif

    QObject::connect(&foxgloveThread, &QThread::started, foxgloveSink, [foxgloveSink]() {
        foxgloveSink->startServer(8765);
        //foxgloveSink->startMcapLogging(true, QStringLiteral("/home/ubcbaja/firmware/logs"));
    });

    QObject::connect(&canThread, &QThread::started, dbcParser, [dbcParser, canSocket]() {
        if (!dbcParser->loadDbc(QStringLiteral(":/roxy.dbc"))) {
            qWarning() << "DBC load failed, not starting CAN";
            return;
        }
        if (!canSocket->start(QString(), QStringLiteral("can0")))
            qWarning() << "CAN start failed";
    });

#ifdef Q_OS_LINUX
    for (auto *w : webcams) QObject::connect(&camThread, &QThread::started, w, &Webcam::begin);
#endif

    engine.rootContext()->setContextProperty("guiBridge", guiBridge);

    foxgloveThread.start();
    canThread.start();
#ifdef Q_OS_LINUX
    camThread.start();
#endif

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/app/src/ui/main.qml")));
    if (engine.rootObjects().isEmpty()) {
#ifdef Q_OS_LINUX
        camThread.quit();   camThread.wait();
#endif
        canThread.quit();   canThread.wait();
        foxgloveThread.quit();  foxgloveThread.wait();
        return -1;
    }

    const int ret = app.exec();

#ifdef Q_OS_LINUX
    camThread.quit();   camThread.wait();
#endif
    canThread.quit();   canThread.wait();
    foxgloveThread.quit();  foxgloveThread.wait();

    return ret;
}