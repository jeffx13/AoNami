#include <QQuickWindow>
#include <QGuiApplication>
#include "app/application.h"
#include "gui/imagenamfactory.h"
//qputenv("QT_DEBUG_PLUGINS", QByteArray("1"));

int main(int argc, char *argv[]){
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/resources/images/icon.png"));

    auto launchPath = QString::fromUtf8(argv[1]);
    Application application(launchPath);

    QQmlApplicationEngine engine;
    engine.setNetworkAccessManagerFactory(new ImageNAMFactory);
    const QUrl url(QStringLiteral("qrc:/src/gui/qml/main.qml"));
    application.setFont(":/resources/app-font.ttf");
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}






