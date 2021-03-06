#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQmlContext>
#include <QObject>
#include "handlers/videohandler.h"
#include "handlers/audiohandler.h"
#include <QVariant>
#include <QScreen>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <QAudioInput>
#include <QTimer>
#include "handlers/outputstreamhandler.h"
#include "handlers/videoplaybackhandler.h"
#include "core/settings.h"
#include "handlers/imagehandler.h"
#include <QQuickView>
#include <stdio.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libavdevice/avdevice.h>
#include <ao/ao.h>
}
#include <QtCore/QCoreApplication>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickView>
#include "handlers/inputstreamhandler.h"
#include "handlers/tcpsockethandler.h"
#include "handlers/audioplaybackhandler.h"
#include "handlers/sessionhandler.h"
#include "handlers/userhandler.h"
#include "handlers/errorhandler.h"

ErrorHandler* errorHandler;

/*! \mainpage qZoom-Client Documentation
 *
 * \section intro_sec Introduction
 *
 * This document describes all the C++ classes used in our qZoom Client
 *
 */

int main(int argc, char *argv[])
{
    avdevice_register_all();

    errorHandler = new ErrorHandler;

    int bufferSize = 10e5;

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName("qZoom-Client");
    QCoreApplication::setApplicationVersion("1.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("Hello! Check out the source code over at https://github.com/Feqzz/qZoom-Client");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption libavPrints("l",
                                 QCoreApplication::translate("main", "Enables libav prints"));
    parser.addOption(libavPrints);
    parser.process(app);

    if (!parser.isSet(libavPrints))
    {
        av_log_set_level(AV_LOG_QUIET);
    }

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/view/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl)
    {
        if (!obj && url == objUrl)
        {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    QScopedPointer<Settings> settings(new Settings());

    ServerTcpQueries* serverTcpQueries = new ServerTcpQueries(settings.data());
    UserHandler* userHandlerObject = new UserHandler(serverTcpQueries, settings.data());
    ImageHandler* imageHandlerObject = new ImageHandler(settings.data());
    SessionHandler* sessionHandlerObject = new SessionHandler(serverTcpQueries, userHandlerObject,
                                                              imageHandlerObject, settings.data(),
                                                              bufferSize);

    QScopedPointer<ImageHandler> imageHandler(imageHandlerObject);
    QScopedPointer<UserHandler> userHandler(userHandlerObject);
    QScopedPointer<SessionHandler> sessionHandler(sessionHandlerObject);

    engine.rootContext()->setContextProperty("imageHandler", imageHandler.data());
    engine.rootContext()->setContextProperty("sessionHandler", sessionHandler.data());
    engine.rootContext()->setContextProperty("backendSettings", settings.data());
    engine.rootContext()->setContextProperty("errorHandler", errorHandler);
    engine.rootContext()->setContextProperty("userHandler", userHandler.data());
    engine.addImageProvider("live", imageHandler.data());

    engine.load(url);
    return app.exec();
}
