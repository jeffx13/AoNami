#pragma once

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <QString>
#include <QVector>
#include <memory>

class Html {
public:
    class Node {
    public:
        Node() = default;
        Node(std::shared_ptr<xmlXPathContext> context, xmlNodePtr node)
            : m_context(std::move(context)), m_node(node) {}

        QVector<Node> select(const QString &xpath) const;
        Node selectFirst(const QString &xpath) const;
        QString text() const;
        QString attr(const QString &name) const;

        explicit operator bool() const { return m_node != nullptr; }

    private:
        // Holding the context keeps the document alive - its deleter owns both.
        std::shared_ptr<xmlXPathContext> m_context;
        xmlNodePtr m_node = nullptr;
    };

    explicit Html(const QString &html);
    static Html parse(const QString &html) { return Html(html); }

    QVector<Node> select(const QString &xpath) const { return m_root.select(xpath); }
    Node selectFirst(const QString &xpath) const { return m_root.selectFirst(xpath); }
    explicit operator bool() const { return bool(m_root); }

private:
    Node m_root;
};
