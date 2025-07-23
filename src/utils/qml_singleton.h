#pragma once
#include <QQmlEngine>

#define DECLARE_QML_SINGLETON(Type) DECLARE_QML_NAMED_SINGLETON(Type,Type) \

#define DECLARE_QML_NAMED_SINGLETON(Type, CustomName) \
struct Type##QmlSingleton\
{\
        Q_GADGET\
        QML_FOREIGN(Type)\
        QML_SINGLETON\
        QML_NAMED_ELEMENT(CustomName)\
    \
        public:\
        static void registerInstance(Type* instance)\
    {\
            /* The instance should be registered with a non-null object and only once. */\
            Q_ASSERT(instance && !s_instance);\
            s_instance = instance;\
            QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);\
    }\
        static Type* create(QQmlEngine*, QJSEngine* engine)\
    {\
            /* The instance has to exist before it is used. */\
            Q_ASSERT(s_instance);\
            /* The engine has to have the same thread affinity as the singleton. */\
            Q_ASSERT(engine->thread() == s_instance->thread());\
            /* There can only be one engine accessing the singleton. */\
            if (s_engine)\
            Q_ASSERT(engine == s_engine);\
            else\
            s_engine = engine;\
            return s_instance;\
    }\
        private:\
        static inline Type* s_instance = nullptr;\
        static inline QJSEngine* s_engine = nullptr;\
};

#define REGISTER_QML_SINGLETON(Type, instance) \
Type##QmlSingleton::registerInstance(instance);
