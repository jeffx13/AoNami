#include <QQuickWindow>
#include <QGuiApplication>
#include "application.h"
//qputenv("QT_DEBUG_PLUGINS", QByteArray("1"));
#include <QDebug>



int main(int argc, char *argv[]){
    //setOneInstance();
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/resources/images/icon.png"));

    Application application(app, QString::fromUtf8(argv[1]));
    //Logger logger;
    //logger.init();

    return app.exec();
}






