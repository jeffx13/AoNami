#include "providers/jsunpack.h"
#include <QRegularExpression>
#include <QJSEngine>
#include <QJSValue>

QString Js::unpack(const QString &html) {
    static QRegularExpression re{R"((\(function\(p,a,c,k,e,d\).*?\))\n<\/script>)"};
    QRegularExpressionMatch packedMatch = re.match(html);
    if (!packedMatch.hasMatch()) return {};
    QString packed = packedMatch.captured(1);

    // Engine is expensive to construct - reuse one per thread.
    static const QString kJs = QStringLiteral(
        "function unPack(c){function i(a){try{var t=0,o=-1,d='';for(var e=0;e<a.length;e++){if(a[e].indexOf('{')!=-1)t++;if(a[e].indexOf('}')!=-1)t--;if(o!=t){o=t;d='';while(o>0){d+='\\t';o--;}o=t;}a[e]=d+a[e];}}finally{t=null;o=null;d=null;}return a;}var e={eval:function(x){c=x;},window:{},document:{}};eval('with(e){'+c+'}');c=(c+'').replace(/;/g,';\\n').replace(/{/g,'\\n{\\n').replace(/}/g,'\\n}\\n').replace(/\\n;\\n/g,';\\n').replace(/\\n\\n/g,'\\n');c=c.split('\\n');c=i(c);c=c.join('\\n');return c;}"
    );
    thread_local QJSEngine tEngine;
    thread_local QJSValue tFn;
    thread_local bool tInitialized = false;
    if (!tInitialized) {
        tEngine.installExtensions(QJSEngine::ConsoleExtension);
        tFn = tEngine.evaluate(kJs + QStringLiteral("\nunPack"));
        tInitialized = tFn.isCallable();
    }
    if (!tFn.isCallable()) return {};
    QJSValue result = tFn.call(QJSValueList{tEngine.toScriptValue(packed)});
    if (result.isError()) return {};
    return result.toString();
}
