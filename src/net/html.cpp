#include "net/html.h"

namespace {

QString take(xmlChar *text) {
    if (!text) return {};
    const QString out = QString::fromUtf8(reinterpret_cast<const char *>(text));
    xmlFree(text);
    return out;
}

// The context node is per-evaluation state, so it has to be set on every call.
struct XPathResult {
    XPathResult(const std::shared_ptr<xmlXPathContext> &context, xmlNodePtr node, const QString &xpath) {
        if (!context || !node) return;
        context->node = node;
        const QByteArray expr = xpath.toUtf8();
        m_object = xmlXPathEvalExpression(reinterpret_cast<const xmlChar *>(expr.constData()), context.get());
    }
    ~XPathResult() { if (m_object) xmlXPathFreeObject(m_object); }
    XPathResult(const XPathResult &) = delete;
    XPathResult &operator=(const XPathResult &) = delete;

    int size() const { return nodes() ? nodes()->nodeNr : 0; }
    xmlNodePtr at(int i) const { return nodes()->nodeTab[i]; }

private:
    xmlNodeSetPtr nodes() const { return m_object ? m_object->nodesetval : nullptr; }
    xmlXPathObjectPtr m_object = nullptr;
};

}

Html::Html(const QString &html) {
    LIBXML_TEST_VERSION

    // Already-decoded text, so UTF-8 by construction; left to sniff, libxml2 falls back to Latin-1.
    const QByteArray bytes = html.toUtf8();
    xmlDocPtr doc = htmlReadMemory(bytes.constData(), bytes.size(), nullptr, "UTF-8",
                                   HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) return;

    xmlXPathContextPtr context = xmlXPathNewContext(doc);
    if (!context) { xmlFreeDoc(doc); return; }

    // Freeing the context also frees its document, so one shared_ptr owns both.
    auto owner = std::shared_ptr<xmlXPathContext>(context, [](xmlXPathContextPtr c) {
        xmlDocPtr owned = c->doc;
        xmlXPathFreeContext(c);
        xmlFreeDoc(owned);
    });
    m_root = Node(std::move(owner), xmlDocGetRootElement(doc));
}

QVector<Html::Node> Html::Node::select(const QString &xpath) const {
    const XPathResult result(m_context, m_node, xpath);
    QVector<Node> nodes;
    nodes.reserve(result.size());
    for (int i = 0; i < result.size(); ++i)
        nodes.emplaceBack(m_context, result.at(i));
    return nodes;
}

Html::Node Html::Node::selectFirst(const QString &xpath) const {
    const XPathResult result(m_context, m_node, xpath);
    return result.size() > 0 ? Node(m_context, result.at(0)) : Node();
}

QString Html::Node::text() const {
    return m_node ? take(xmlNodeGetContent(m_node)) : QString();
}

QString Html::Node::attr(const QString &name) const {
    if (!m_node) return {};
    const QByteArray utf8 = name.toUtf8();
    return take(xmlGetProp(m_node, reinterpret_cast<const xmlChar *>(utf8.constData())));
}
