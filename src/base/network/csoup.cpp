#include "csoup.h"
#include "app/logger.h"

CSoup::CSoup(const QString &htmlContent) {
    LIBXML_TEST_VERSION

    QByteArray byteArray = htmlContent.toUtf8();
    docPtr = std::shared_ptr<xmlDoc>(
        htmlReadMemory(byteArray.constData(), byteArray.size(), nullptr, nullptr,
                       HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING),
        xmlFreeDoc
    );

    if (!docPtr) {
        oLog() << "CSoup" << "Failed to parse HTML";
    }

    contextPtr = std::shared_ptr<xmlXPathContext>(
        xmlXPathNewContext(docPtr.get()), xmlXPathFreeContext
    );
    if (!contextPtr) {
        oLog() << "CSoup" << "Failed to create XPath context";
        docPtr = nullptr;
    }
}

QVector<CSoup::Node> CSoup::select(std::shared_ptr<xmlDoc> docPtr, std::shared_ptr<xmlXPathContext> contextPtr, xmlNodePtr nodePtr, const QString &xpathExpr) {
    QVector<Node> nodes;
    if (!contextPtr) {
        oLog() << "CSoup" << "Null XPath context in select";
        return nodes;
    }
    contextPtr->node = nodePtr;
    xmlXPathObjectPtr result = executeXPath(contextPtr, xpathExpr);
    if (result && result->nodesetval) {
        for (int i = 0; i < result->nodesetval->nodeNr; ++i) {
            nodes.emplaceBack(docPtr, contextPtr, result->nodesetval->nodeTab[i]);
        }
    }
    if (result) {
        xmlXPathFreeObject(result);
    }
    if (nodes.isEmpty()) {
        oLog() << "CSoup" << "No matching node set found for" << xpathExpr;
    }
    return nodes;
}

CSoup::Node CSoup::selectNth(std::shared_ptr<xmlDoc> docPtr, std::shared_ptr<xmlXPathContext> contextPtr, xmlNodePtr nodePtr, const QString &xpathExpr, int n, bool reversed) {
    if (!contextPtr) {
        oLog() << "CSoup" << "Null XPath context in selectNth";
        return Node();
    }
    contextPtr->node = nodePtr;
    xmlXPathObjectPtr result = executeXPath(contextPtr, xpathExpr);
    if (result && result->nodesetval && result->nodesetval->nodeNr > 0 && n >= 0 && n < result->nodesetval->nodeNr) {
        int idx = reversed ? result->nodesetval->nodeNr - 1 - n : n;
        xmlNodePtr node = result->nodesetval->nodeTab[idx];
        xmlXPathFreeObject(result);
        return Node(docPtr, contextPtr, node);
    }
    if (result) {
        xmlXPathFreeObject(result);
    }
    oLog() << "CSoup" << "No matching node found for" << xpathExpr;
    return Node();
}

xmlXPathObjectPtr CSoup::executeXPath(std::shared_ptr<xmlXPathContext> context, const QString &xpathExpr) {
    if (!context) return nullptr;
    QByteArray xpathExprUtf8 = xpathExpr.toUtf8();
    xmlXPathObjectPtr result = xmlXPathEvalExpression(
        reinterpret_cast<const xmlChar*>(xpathExprUtf8.constData()), context.get()
    );
    if (!result) {
        oLog() << "CSoup" << "Failed to evaluate XPath expression" << xpathExpr;
    }
    return result;
}
