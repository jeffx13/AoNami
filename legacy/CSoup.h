// #pragma once
// #include <QString>
// #include <QDebug>
// #include <tidy.h>
// #include <tidybuffio.h>
// #include <pugixml/pugixml.hpp>

// class CSoup
// {
// private:
//     pugi::xml_document doc;
// public:
//     CSoup(const QString& html);

//     ~CSoup() = default;

//     const char* selectText(const std::string& XPath) const;
//     inline pugi::xpath_node_set select(const std::string& XPath) const {
//         return doc.select_nodes(XPath.data());
//     }
//     inline pugi::xpath_node selectFirst(const std::string& XPath) const {
//         return doc.select_node(XPath.data());
//     }

// private:
//     std::string tidy(const char* input);
//     bool parse (const char* source);
// };

