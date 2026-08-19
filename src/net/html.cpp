#include "net/html.h"

Html::Html(const QString &htmlContent) {
    LIBXML_TEST_VERSION

    // The input is already decoded text, so the bytes are UTF-8 by construction. Left to sniff,
    // libxml2 reads a late <meta charset> too slowly and falls back to Latin-1, mangling CJK.
    QByteArray bytes = htmlContent.toUtf8();
    docPtr = std::shared_ptr<xmlDoc>(
        htmlReadMemory(bytes.constData(), bytes.size(), nullptr, "UTF-8",
                       HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING),
        xmlFreeDoc);

    if (!docPtr) return;

    contextPtr = std::shared_ptr<xmlXPathContext>(
        xmlXPathNewContext(docPtr.get()), xmlXPathFreeContext);

    if (!contextPtr) docPtr = nullptr;
}

QVector<Html::Node> Html::select(
    std::shared_ptr<xmlDoc> docPtr,
    std::shared_ptr<xmlXPathContext> contextPtr,
    xmlNodePtr nodePtr,
    const QString &xpathExpr)
{
    QVector<Node> nodes;
    if (!contextPtr) return nodes;

    contextPtr->node = nodePtr;
    xmlXPathObjectPtr result = executeXPath(contextPtr, xpathExpr);
    if (result && result->nodesetval) {
        for (int i = 0; i < result->nodesetval->nodeNr; ++i)
            nodes.emplaceBack(docPtr, contextPtr, result->nodesetval->nodeTab[i]);
    }
    if (result) xmlXPathFreeObject(result);
    return nodes;
}

Html::Node Html::selectNth(
    std::shared_ptr<xmlDoc> docPtr,
    std::shared_ptr<xmlXPathContext> contextPtr,
    xmlNodePtr nodePtr,
    const QString &xpathExpr,
    int n,
    bool reversed)
{
    if (!contextPtr) return {};

    contextPtr->node = nodePtr;
    xmlXPathObjectPtr result = executeXPath(contextPtr, xpathExpr);
    if (result && result->nodesetval && n >= 0 && n < result->nodesetval->nodeNr) {
        int idx = reversed ? result->nodesetval->nodeNr - 1 - n : n;
        xmlNodePtr node = result->nodesetval->nodeTab[idx];
        xmlXPathFreeObject(result);
        return Node(docPtr, contextPtr, node);
    }
    if (result) xmlXPathFreeObject(result);
    return {};
}

xmlXPathObjectPtr Html::executeXPath(std::shared_ptr<xmlXPathContext> context, const QString &xpathExpr) {
    if (!context) return nullptr;
    QByteArray bytes = xpathExpr.toUtf8();
    return xmlXPathEvalExpression(reinterpret_cast<const xmlChar *>(bytes.constData()), context.get());
}
