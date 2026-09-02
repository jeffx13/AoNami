#pragma once

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <QString>
#include <QVector>
#include <memory>

class Html {
public:
    explicit Html(const QString &htmlContent);

    Html(const Html&) = delete;
    Html& operator=(const Html&) = delete;
    Html(Html&&) = delete;
    Html& operator=(Html&&) = delete;

    static Html parse(const QString &htmlContent) {
        return Html(htmlContent);
    }

    struct Node {
        Node(std::shared_ptr<xmlDoc> docPtr = nullptr,
             std::shared_ptr<xmlXPathContext> contextPtr = nullptr,
             xmlNodePtr nodePtr = nullptr)
            : m_docPtr(std::move(docPtr)), m_contextPtr(std::move(contextPtr)), m_nodePtr(nodePtr) {}

        Node(const Node&) = default;
        Node& operator=(const Node&) = default;

        Node selectFirst(const QString &xpathExpr) const {
            if (!m_nodePtr) return Node();
            return Html::selectFirst(m_docPtr, m_contextPtr, m_nodePtr, xpathExpr);
        }
        QString text() const {
            if (!m_nodePtr) return {};
            xmlChar *content = xmlNodeGetContent(m_nodePtr);
            QString nodeContent;
            if (content) {
                nodeContent = QString::fromUtf8(reinterpret_cast<const char*>(content));
                xmlFree(content);
            }
            return nodeContent;
        }
        QString attr(const QString &attrName) const {
            if (!m_nodePtr) return {};
            QByteArray attrNameUtf8 = attrName.toUtf8();
            xmlChar *attr = xmlGetProp(m_nodePtr, reinterpret_cast<const xmlChar*>(attrNameUtf8.constData()));
            QString attrValue;
            if (attr) {
                attrValue = QString::fromUtf8(reinterpret_cast<const char*>(attr));
                xmlFree(attr);
            }
            return attrValue;
        }
        QVector<Node> select(const QString &xpathExpr) const {
            if (!m_nodePtr) return {};
            return Html::select(m_docPtr, m_contextPtr, m_nodePtr, xpathExpr);
        }
        explicit operator bool() const {
            return m_nodePtr != nullptr;
        }
    private:
        std::shared_ptr<xmlDoc> m_docPtr;
        std::shared_ptr<xmlXPathContext> m_contextPtr;
        xmlNodePtr m_nodePtr = nullptr;
    };

    Node selectFirst(const QString &xpathExpr) const {
        if (!docPtr) return Node();
        return Html::selectFirst(docPtr, contextPtr, nullptr, xpathExpr);
    }
    QVector<Node> select(const QString &xpathExpr) const {
        if (!docPtr) return {};
        return Html::select(docPtr, contextPtr, nullptr, xpathExpr);
    }
    explicit operator bool() const {
        return docPtr && contextPtr;
    }

private:
    static QVector<Node> select(std::shared_ptr<xmlDoc> docPtr,
                                std::shared_ptr<xmlXPathContext> contextPtr,
                                xmlNodePtr nodePtr,
                                const QString &xpathExpr);

    static Node selectFirst(std::shared_ptr<xmlDoc> docPtr,
                            std::shared_ptr<xmlXPathContext> contextPtr,
                            xmlNodePtr nodePtr,
                            const QString &xpathExpr);

    static xmlXPathObjectPtr executeXPath(std::shared_ptr<xmlXPathContext> context,
                                          const QString &xpathExpr);

    std::shared_ptr<xmlDoc> docPtr = nullptr;
    std::shared_ptr<xmlXPathContext> contextPtr = nullptr;
};
