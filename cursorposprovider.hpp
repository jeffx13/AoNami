#ifndef CURSORPOSPROVIDER_HPP
#define CURSORPOSPROVIDER_HPP

#include <QCursor>
#include <QObject>
#include <QPointF>



class CursorPosProvider : public QObject
{
    Q_OBJECT
public:
    explicit CursorPosProvider(QObject *parent = nullptr) : QObject(parent)
    {
    }
    virtual ~CursorPosProvider() = default;

    Q_INVOKABLE QPointF cursorPos()
    {
        return QCursor::pos();
    }
};

#endif // CURSORPOSPROVIDER_HPP
