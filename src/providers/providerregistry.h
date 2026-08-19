#pragma once
#include <QList>
#include <functional>
#include <algorithm>

class ShowProvider;
class QObject;

// Provider self-registration via REGISTER_PROVIDER(Class, order); order = display order.
class ProviderRegistry {
public:
    using Factory = std::function<ShowProvider *(QObject *)>;

    struct Entry { int order; Factory factory; };

    static QList<Entry> &entries() {
        static QList<Entry> e;   // function-local static avoids static-init-order issues
        return e;
    }

    static void add(int order, Factory factory) {
        entries().append({order, std::move(factory)});
    }

    static QList<ShowProvider *> createAll(QObject *parent) {
        QList<Entry> sorted = entries();
        std::stable_sort(sorted.begin(), sorted.end(),
                         [](const Entry &a, const Entry &b) { return a.order < b.order; });
        QList<ShowProvider *> providers;
        providers.reserve(sorted.size());
        for (const Entry &e : sorted)
            providers.append(e.factory(parent));
        return providers;
    }
};

#define REGISTER_PROVIDER(Class, order)                                        \
    namespace {                                                                \
    const bool _aonami_reg_##Class = (ProviderRegistry::add(                   \
        (order),                                                               \
        [](QObject *parent) { return static_cast<ShowProvider *>(new Class(parent)); }), \
        true);                                                                 \
    }
