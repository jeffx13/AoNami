#ifndef GLOBAL_H
#define GLOBAL_H

#include <parsers/showparser.h>
#include <parsers/showresponse.h>

#include <parsers/providers/gogoanime.h>
#include <parsers/providers/nivod.h>
#include <parsers/providers/consumet/consumet9anime.h>

class Global : public QObject {
    Q_OBJECT
    Q_PROPERTY(ShowResponse currentShow READ currentShow NOTIFY currentShowChanged NOTIFY currentShowPropertyChanged)
    Q_PROPERTY(ShowResponseObject* currentShowObject READ currentShowObject CONSTANT)
    Q_PROPERTY(QList<ShowParser*> providers READ providers CONSTANT)
    Q_PROPERTY(ShowParser* currentSearchProvider READ getCurrentShowProvider CONSTANT)

    QMap<int,ShowParser*> providersMap{
        {Providers::e_Nivod,new Nivod},
        {Providers::e_Consumet9anime,new Consumet9anime},
        {Providers::e_Gogoanime,new Gogoanime}
    };
    ShowResponseObject m_currentShowObject;
    ShowParser* m_currentSearchProvider = nullptr;
public:

    inline ShowResponseObject* currentShowObject() {
        return &m_currentShowObject;
    };

    inline ShowParser* getProvider(int provider) const {
        return providersMap[provider];
    }

    inline ShowParser* getCurrentShowProvider() {
        return providersMap[m_currentShowObject.getShow()->provider];
    }

    inline ShowParser* getCurrentSearchProvider() {
        Q_ASSERT(m_currentSearchProvider!=nullptr);
        return m_currentSearchProvider;
    }

    inline QList<ShowParser*> providers() const {
        return providersMap.values ();
    }

    inline Q_INVOKABLE void changeSearchProvider(int providerEnum) {
        m_currentSearchProvider = providersMap[providerEnum];
        emit currentShowProviderChanged();
    }

    inline ShowResponse currentShow(){
        return *m_currentShowObject.getShow ();
    };

    static Global& instance() {
        static Global instance;
        return instance;
    }

signals:
    void currentShowChanged(void);
    void currentShowPropertyChanged(void);
    void currentShowProviderChanged(void);
private:
    Global() {
        m_currentSearchProvider = providersMap[Providers::e_Nivod];
        connect(&m_currentShowObject, &ShowResponseObject::showChanged,this, [&](){
            emit currentShowChanged();
        });


    }

    ~Global(){
        m_currentSearchProvider=nullptr;
        for (auto it = providersMap.begin(); it != providersMap.end(); ++it) {
            delete it.value();
        }
        providersMap.clear ();
    }

    Global(const Global&) = delete; // delete copy constructor
    Global& operator=(const Global&) = delete; // delete copy assignment operator

};
#endif // GLOBAL_H
