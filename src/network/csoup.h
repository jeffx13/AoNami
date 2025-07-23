#pragma once


#include "utils/logger.h"
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <QString>
#include <QVector>
#include <QDebug>

class CSoup {
    CSoup(const QString &htmlContent);


    std::shared_ptr<xmlDoc> docPtr = nullptr;
    std::shared_ptr<xmlXPathContext> contextPtr = nullptr;
    // xmlNodePtr nodePtr = nullptr;


public:
    static CSoup parse(const QString &htmlContent) {
        return CSoup(htmlContent);
    }

    // Deleting copy constructor and copy assignment operator
    CSoup(const CSoup&) = delete;
    CSoup& operator=(const CSoup&) = delete;

    // Deleting move constructor and move assignment operator
    CSoup(CSoup&&) = delete;
    CSoup& operator=(CSoup&&) = delete;

    struct Node {

        Node(std::shared_ptr<xmlDoc> docPtr = nullptr, std::shared_ptr<xmlXPathContext> contextPtr = nullptr, xmlNodePtr nodePtr = nullptr)
            : m_nodePtr(nodePtr), m_docPtr(docPtr), m_contextPtr(contextPtr) {}

        // Default copy constructor and assignment operator
        Node(const Node&) = default;
        Node& operator=(const Node&) = default;

        inline Node selectFirst(const QString &xpathExpr) const {
            return selectNth(xpathExpr, 0);
        }

        inline Node selectLast(const QString &xpathExpr) const {
            return selectNth(xpathExpr, 0, true);
        }

        Node selectNth(const QString &xpathExpr, int n, bool reversed = false) const {
            if (!m_nodePtr) return Node();
            return CSoup::selectNth(m_docPtr, m_contextPtr, m_nodePtr, xpathExpr, n, reversed);
        }

        QString text() const {
            if (!m_nodePtr) return "";
            xmlChar *content = xmlNodeGetContent(m_nodePtr);
            if (content) {
                QString nodeContent = QString::fromUtf8(reinterpret_cast<const char*>(content));
                xmlFree(content);
                return nodeContent;
            }
            return "";
        }

        QString attr(const QString &attrName) const {
            if (!m_nodePtr) return "";
            QByteArray attrNameUtf8 = attrName.toUtf8();
            xmlChar *attr = xmlGetProp(m_nodePtr, reinterpret_cast<const xmlChar*>(attrNameUtf8.constData()));
            if (attr) {
                QString attrValue = QString::fromUtf8(reinterpret_cast<const char*>(attr));
                xmlFree(attr);
                return attrValue;
            }
            return "";
        }

        QVector<Node> select(const QString &xpathExpr) const {
            if (!m_nodePtr) return {};
            return CSoup::select(m_docPtr, m_contextPtr, m_nodePtr, xpathExpr);
        }

        void print() const {
            if (!m_nodePtr) return;

            xmlBufferPtr buffer = xmlBufferCreate();
            if (buffer == nullptr) {
                throw std::runtime_error("Failed to create xmlBuffer");
            }
            if (xmlNodeDump(buffer, m_docPtr.get(), m_nodePtr, 0, 1) == -1) {
                xmlBufferFree(buffer);
                throw std::runtime_error("Failed to dump XML node");
            }
            QString nodeContent = QString::fromUtf8(reinterpret_cast<const char*>(buffer->content));
            xmlBufferFree(buffer);
            cLog() << "CSoup" <<  nodeContent;
        }

        explicit operator bool() const {
            return  m_nodePtr != nullptr ||  m_docPtr != nullptr ||  m_contextPtr != nullptr;
        }

    private:
        std::shared_ptr<xmlDoc> m_docPtr = nullptr;
        std::shared_ptr<xmlXPathContext> m_contextPtr = nullptr;
        xmlNodePtr m_nodePtr = nullptr;
    };


    inline Node selectFirst(const QString &xpathExpr) const {
        return selectNth(xpathExpr, 0);
    }
    inline Node selectLast(const QString &xpathExpr) const {
        return selectNth(xpathExpr, 0, true);
    }

    Node selectNth(const QString &xpathExpr, int n, bool reversed=false) const {
        if (!docPtr) return Node();
        return CSoup::selectNth(docPtr, contextPtr, nullptr, xpathExpr, 0, reversed);
    }

    QVector<Node> select(const QString &xpathExpr) const {
        if (!docPtr) return {};
        return CSoup::select(docPtr, contextPtr, nullptr, xpathExpr);
    }

    explicit operator bool() const {return docPtr && contextPtr;}


private:
    static QVector<Node> select(std::shared_ptr<xmlDoc> docPtr, std::shared_ptr<xmlXPathContext> contextPtr, xmlNodePtr nodePtr, const QString &xpathExpr);

    static Node selectNth(std::shared_ptr<xmlDoc> docPtr, std::shared_ptr<xmlXPathContext> contextPtr, xmlNodePtr nodePtr,  const QString &xpathExpr, int n, bool reversed = false);

    static xmlXPathObjectPtr executeXPath(std::shared_ptr<xmlXPathContext> context, const QString &xpathExpr);
};




