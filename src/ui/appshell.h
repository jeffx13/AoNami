#pragma once
#include <QString>
#include <QObject>
#include <QEvent>
#include <QMouseEvent>
#include <QWindow>
#include <QCoreApplication>
#include "app/qmlsingleton.h"

class AppShell : public QObject
{
    Q_OBJECT
public:
    enum Page {
        Search = 0,
        Info = 1,
        Library = 2,
        Player = 3,
        Download = 4,
        Log = 5,
        Settings = 6,
        History = 7
    };
    Q_ENUM(Page)

public:
    void reportError(const QString &message, const QString &header = "Error") {
        emit errorReported(message, header);
    }

    void reportInfo(const QString &message, const QString &header = "Info") {
        emit infoReported(message, header);
    }

    void navigateTo(Page page) {
        emit navigateRequested(page);
    }

    // Not an item in the scene: anything covering the window would own the cursor.
    void installBackForwardFilter() { QCoreApplication::instance()->installEventFilter(this); }

    static AppShell &instance() {
        static AppShell shell;
        return shell;
    }

signals:
    void errorReported(const QString &message, const QString &header);
    void infoReported(const QString &message, const QString &header);
    void navigateRequested(AppShell::Page page);
    void historyStepRequested(int delta);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() != QEvent::MouseButtonPress || !qobject_cast<QWindow *>(watched))
            return false;
        const Qt::MouseButton button = static_cast<QMouseEvent *>(event)->button();
        if (button != Qt::BackButton && button != Qt::ForwardButton)
            return false;
        emit historyStepRequested(button == Qt::ForwardButton ? 1 : -1);
        return true;
    }

private:
    AppShell() = default;
    AppShell(const AppShell&) = delete;
    AppShell& operator=(const AppShell&) = delete;
    ~AppShell() = default;
};

DECLARE_QML_SINGLETON(AppShell);
