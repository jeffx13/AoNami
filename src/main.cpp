#include <QQuickWindow>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include "app/application.h"
#include "ui/imagenamfactory.h"

#ifdef _WIN32
// Prefer the discrete GPU on switchable-graphics laptops.
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_ACCESSIBILITY", "0");

    // Threaded render loop (Qt defaults to the basic single-thread loop on Windows+GL).
    qputenv("QSG_RENDER_LOOP", "threaded");

    // Shared GL contexts so the mpv render thread's textures are sampleable by QML.
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/AoNami/resources/app.ico"));

    QString launchPath = (argc > 1) ? QString::fromUtf8(argv[1]) : QString();
    Application application(launchPath);
    application.setFont(":/AoNami/resources/app-font.ttf");

    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/AoNami/src/ui/qml");
    engine.setNetworkAccessManagerFactory(new ImageNAMFactory);

    const QUrl url(QStringLiteral("qrc:/AoNami/src/ui/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [&url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);
    engine.load(url);
    return app.exec();
}
