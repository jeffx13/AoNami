#ifndef VIDSTREAM_H
#define VIDSTREAM_H

#include "parsers/providers/nineanime.h"
#include "videoextractor.h"



class Vidstream: public VideoExtractor
{
public:
    Vidstream();

//    NineAnime::Keys keys;
public:
    bool extract(VideoServer *server){
        std::string serverUrl = "https://vidstream.pro";
        std::string  referer = "https://vidstream.pro/embed/$cloudId?autostart=true";
        auto id = Functions::base64Encode(Functions::rc4("sd", server->link),"encryptKey");
//        auto encrypted = NineAnime::encrypt (id,keys);
        return true;
    }
};

#endif // VIDSTREAM_H
