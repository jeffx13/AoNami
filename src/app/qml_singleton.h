#pragma once
#include <QQmlEngine>
#include <qqmlintegration.h>

// Simple QML singleton - Type name matches QML element name
#define DECLARE_QML_SINGLETON(Type) DECLARE_QML_NAMED_SINGLETON(Type, Type)

// Named QML singleton - allows a different QML element name
#define DECLARE_QML_NAMED_SINGLETON(Type, CustomName) \
struct Type##QmlSingleton \
{ \
        Q_GADGET \
        QML_FOREIGN(Type) \
        QML_SINGLETON \
        QML_NAMED_ELEMENT(CustomName) \
\
    public: \
    static void registerInstance(Type *instance) \
    { \
            Q_ASSERT(instance && !s_instance); \
            s_instance = instance; \
            QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership); \
    } \
        static Type *create(QQmlEngine *, QJSEngine *engine) \
    { \
            Q_ASSERT(s_instance); \
            Q_ASSERT(engine->thread() == s_instance->thread()); \
            if (s_engine) Q_ASSERT(engine == s_engine); \
            else s_engine = engine; \
            return s_instance; \
    } \
\
    private: \
    static inline Type *s_instance = nullptr; \
        static inline QJSEngine *s_engine = nullptr; \
};

#define REGISTER_QML_SINGLETON(Type, instance) \
Type##QmlSingleton::registerInstance(instance);
