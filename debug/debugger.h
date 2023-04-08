#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <QString>
#include<QDebug>


class Debugger
{
public:
    Debugger();
    void popUp();
    void info(const char * text){
        qInfo( "%s", text );
    }

};

#endif // DEBUGGER_H
