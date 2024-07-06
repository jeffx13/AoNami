#include "csoup.h"

CSoup::CSoup(const QString &htmlContent) {
    xmlInitParser();
    LIBXML_TEST_VERSION
    QByteArray byteArray = htmlContent.toUtf8();
    xmlPtrs.docPtr = htmlReadMemory(byteArray.constData(), byteArray.size(), nullptr, nullptr, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (xmlPtrs.docPtr == nullptr) {
        qDebug() << "Failed to parse HTML";
    }
    xmlPtrs.contextPtr = xmlXPathNewContext(xmlPtrs.docPtr);
    if (xmlPtrs.contextPtr == nullptr) {
        xmlFreeDoc(xmlPtrs.docPtr);
        qDebug() << "Failed to create XPath context";
    }
}

CSoup::~CSoup() {
    if (xmlPtrs.contextPtr) {
        xmlXPathFreeContext(xmlPtrs.contextPtr);
    }
    if (xmlPtrs.docPtr) {
        xmlFreeDoc(xmlPtrs.docPtr);
    }
    xmlCleanupParser();
}

QVector<CSoup::Node> CSoup::select(const XmlPtrs &xmlPtrs, const QString &xpathExpr) {
    QVector<Node> nodes;
    xmlPtrs.contextPtr->node = xmlPtrs.nodePtr;
    xmlXPathObjectPtr result = executeXPath(xmlPtrs.contextPtr, xpathExpr);
    if (result && result->nodesetval) {
        for (int i = 0; i < result->nodesetval->nodeNr; ++i) {
            nodes.emplaceBack(result->nodesetval->nodeTab[i], xmlPtrs.docPtr, xmlPtrs.contextPtr);
        }
    }
    if (result) {
        xmlXPathFreeObject(result);
    }
    if (nodes.isEmpty())
        qDebug() << "No matching node set found for" << xpathExpr;
    return nodes;
}

CSoup::Node CSoup::selectNth(const XmlPtrs &xmlPtrs, const QString &xpathExpr, int n, bool reversed) {
    xmlPtrs.contextPtr->node = xmlPtrs.nodePtr;
    xmlXPathObjectPtr result =  executeXPath(xmlPtrs.contextPtr, xpathExpr);
    if (result && result->nodesetval && result->nodesetval->nodeNr > 0 && n >= 0 && n < result->nodesetval->nodeNr) {
        n = reversed ? result->nodesetval->nodeNr - n : n;
        xmlNodePtr node = result->nodesetval->nodeTab[n];
        xmlXPathFreeObject(result);
        return Node(node, xmlPtrs.docPtr, xmlPtrs.contextPtr);
    }
    if (result) {
        xmlXPathFreeObject(result);
    }
    qDebug() << "No matching node found for" << xpathExpr;
    return Node();
}

xmlXPathObjectPtr CSoup::executeXPath(xmlXPathContextPtr context, const QString &xpathExpr) {
    if (!context) return nullptr;
    QByteArray xpathExprUtf8 = xpathExpr.toUtf8();
    xmlXPathObjectPtr result = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(xpathExprUtf8.constData()), context);
    if (result == nullptr) {
        qDebug() << "Failed to evaluate XPath expression" << xpathExprUtf8;
    }
    return result;
}
