#include <QQuickWindow>
#include <QGuiApplication>
#include "application.h"
//qputenv("QT_DEBUG_PLUGINS", QByteArray("1"));


int main(int argc, char *argv[]){
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/resources/images/icon.png"));

    auto launchPath = QString::fromUtf8(argv[1]);
    Application application(app, launchPath);
    return app.exec();
}





