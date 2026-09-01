#pragma once
#include <QString>
#include <QObject>
#include <QEvent>
#include <QMouseEvent>
#include <QWindow>
#include <QCoreApplication>
#include "app/qmlsingleton.h"

class UiBridge : public QObject
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
    void showError(const QString &message, const QString &header = "Error") {
        emit errorOccurred(message, header);
    }

    void showInfo(const QString &message, const QString &header = "Info") {
        emit infoOccurred(message, header);
    }

    void navigateTo(Page page) {
        emit navigateRequested(page);
    }

    // Not an item in the scene: anything covering the window also owns the cursor, so every
    // button underneath lost its pointing hand.
    void watchMouseNavigation() { QCoreApplication::instance()->installEventFilter(this); }

    static UiBridge &instance() {
        static UiBridge handler;
        return handler;
    }

signals:
    void errorOccurred(const QString &message, const QString &header);
    void infoOccurred(const QString &message, const QString &header);
    void navigateRequested(UiBridge::Page page);
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
    UiBridge() = default;
    UiBridge(const UiBridge&) = delete;
    UiBridge& operator=(const UiBridge&) = delete;
    ~UiBridge() = default;
};

DECLARE_QML_NAMED_SINGLETON(UiBridge, UiBridge);
