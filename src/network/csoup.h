#pragma once
#include "network.h"

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <QString>
#include <QVector>
#include <QDebug>

class CSoup {
    CSoup(const QString &htmlContent);

    struct XmlPtrs {
        xmlDocPtr docPtr = nullptr;
        xmlXPathContextPtr contextPtr = nullptr;
        xmlNodePtr nodePtr = nullptr;
        ~XmlPtrs(){}
    };
    XmlPtrs xmlPtrs;

public:
    static CSoup parse(const QString &htmlContent) {
        return CSoup(htmlContent);
    }
    static CSoup connect(const QString &url, const QMap<QString, QString> &headers = {}) {
        auto response = NetworkClient::get(url, headers);
        return CSoup(response.body);
    }

    ~CSoup();

    struct Node {

        Node(xmlNodePtr nodePtr = nullptr, xmlDocPtr docPtr = nullptr, xmlXPathContextPtr ctxPtr = nullptr)
            : xmlPtrs(docPtr,ctxPtr, nodePtr) {}

        inline Node selectFirst(const QString &xpathExpr) const {
            return selectNth(xpathExpr, 0);
        }

        inline Node selectLast(const QString &xpathExpr) const {
            return selectNth(xpathExpr, 0, true);
        }
        Node selectNth(const QString &xpathExpr, int n, bool reversed = false) const {
            if (!xmlPtrs.nodePtr) return Node();
            return CSoup::selectNth(xmlPtrs, xpathExpr, 0, reversed);
        }

        QString text() const {
            if (!xmlPtrs.nodePtr) return "";
            xmlChar *content = xmlNodeGetContent(xmlPtrs.nodePtr);
            if (content) {
                QString nodeContent = QString::fromUtf8(reinterpret_cast<const char*>(content));
                xmlFree(content);
                return nodeContent;
            }
            return "";
        }

        QString attr(const QString &attrName) const {
            if (!xmlPtrs.nodePtr) return "";
            QByteArray attrNameUtf8 = attrName.toUtf8();
            xmlChar *attr = xmlGetProp(xmlPtrs.nodePtr, reinterpret_cast<const xmlChar*>(attrNameUtf8.constData()));
            if (attr) {
                QString attrValue = QString::fromUtf8(reinterpret_cast<const char*>(attr));
                xmlFree(attr);
                return attrValue;
            }
            return "";
        }

        QVector<Node> select(const QString &xpathExpr) const {
            if (!xmlPtrs.nodePtr) return {};
            return CSoup::select(xmlPtrs, xpathExpr);
        }

        void print() const {
            if (!xmlPtrs.nodePtr) return;

            xmlBufferPtr buffer = xmlBufferCreate();
            if (buffer == nullptr) {
                throw std::runtime_error("Failed to create xmlBuffer");
            }
            if (xmlNodeDump(buffer, xmlPtrs.docPtr, xmlPtrs.nodePtr, 0, 1) == -1) {
                xmlBufferFree(buffer);
                throw std::runtime_error("Failed to dump XML node");
            }
            QString nodeContent = QString::fromUtf8(reinterpret_cast<const char*>(buffer->content));
            xmlBufferFree(buffer);
            qDebug() << nodeContent;
        }

        explicit operator bool() const {
            return xmlPtrs.nodePtr != nullptr || xmlPtrs.docPtr != nullptr || xmlPtrs.contextPtr != nullptr;
        }
    private:
        XmlPtrs xmlPtrs;
    };


    inline Node selectFirst(const QString &xpathExpr) const {
        return selectNth(xpathExpr, 0);
    }
    inline Node selectLast(const QString &xpathExpr) const {
        return selectNth(xpathExpr, 0, true);
    }

    Node selectNth(const QString &xpathExpr, int n, bool reversed=false) const {
        if (!xmlPtrs.docPtr) return Node();
        return CSoup::selectNth(xmlPtrs, xpathExpr, 0, reversed);
    }

    QVector<Node> select(const QString &xpathExpr) const {
        if (!xmlPtrs.docPtr) return {};
        return CSoup::select(xmlPtrs, xpathExpr);
    }

    explicit operator bool() const {return xmlPtrs.docPtr != nullptr && xmlPtrs.contextPtr != nullptr;}


private:
    static QVector<Node> select(const XmlPtrs &xmlPtrs, const QString &xpathExpr);

    static Node selectNth(const XmlPtrs &xmlPtrs, const QString &xpathExpr, int n, bool reversed = false);

    static xmlXPathObjectPtr executeXPath(xmlXPathContextPtr context, const QString &xpathExpr);
};
