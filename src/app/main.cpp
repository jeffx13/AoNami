#include <QQuickWindow>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include "app/application.h"
#include <QStringLiteral>
#include "ui/imagenamfactory.h"
//qputenv("QT_DEBUG_PLUGINS", QByteArray("1"));

int main(int argc, char *argv[]){
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/AoNami/resources/app.ico"));
    QString launchPath = (argc > 1 ? QString::fromUtf8(argv[1]) : QString());
    Application application(launchPath);
    application.setFont(":/AoNami/resources/app-font.ttf");

    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/AoNami/src/ui/qml");

    engine.setNetworkAccessManagerFactory(new ImageNAMFactory);

    auto url = QUrl(QStringLiteral("qrc:/AoNami/src/ui/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);
    engine.load(url);
    return app.exec();
}






