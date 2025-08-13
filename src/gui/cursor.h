#pragma once
#include <QCursor>
#include <QGuiApplication>
#include <QObject>
#include <QPointF>

class Cursor : public QObject
{
    Q_OBJECT

    // Q_PROPERTY(Qt::CursorShape shape READ getCursorShape WRITE setCursorShape)
    // bool isVisible() { return shape != Qt::BlankCursor; }
    // void setVisible(bool visible) {
    //     if (visible) {
    //         QGuiApplication::setOverrideCursor(QCursor(shape));
    //     } else {
    //         shape = QGuiApplication::overrideCursor()->shape();
    //         QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
    //     }
    // }
private:
    Qt::CursorShape shape = Qt::ArrowCursor;
public:
    explicit Cursor(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QPointF pos() const {
        return QCursor::pos();
    }
    // void setCursorShape(Qt::CursorShape cursorShape) {
    //     QGuiApplication::setOverrideCursor(QCursor(cursorShape));
    // }
    // Qt::CursorShape getCursorShape() {
    //     return QGuiApplication::overrideCursor()->shape();
    // }
};
