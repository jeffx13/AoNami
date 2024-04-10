
#ifndef MYEXCEPTION_H
#define MYEXCEPTION_H

#include <QException>
#include <QDebug>
class MyException : public QException
{
public:
    MyException(const QString& message) : m_message(message){}
    void raise() const override { throw *this; }
    QException* clone() const override { return new MyException(*this); }

private:
    QString m_message;

public:
    const char *what() const noexcept override{
        return m_message.toLocal8Bit().data();
    };
};

#endif // MYEXCEPTION_H
