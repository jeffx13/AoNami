#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <QCursor>
#include <QObject>
#include <QPointF>



class CursorPosProvider : public QObject
{
    Q_OBJECT
public:
    explicit CursorPosProvider(QObject *parent = nullptr) : QObject(parent)
    {}

    Q_INVOKABLE QPointF cursorPos()
    {
        return QCursor::pos();
    }
    static CursorPosProvider& instance()
    {
        static CursorPosProvider s_instance;
        return s_instance;
    }
    ~CursorPosProvider() {} // Private destructor to prevent external deletion.
    CursorPosProvider(const ApplicationModel&) = delete; // Disable copy constructor.
    CursorPosProvider& operator=(const ApplicationModel&) = delete; // Disable copy assignment.
};

#endif // UTILS_H
