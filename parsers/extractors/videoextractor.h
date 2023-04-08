#ifndef VIDEOEXTRACTOR_H
#define VIDEOEXTRACTOR_H

#include <parsers/episode.h>
#include <functions.hpp>


class VideoExtractor
{
private:
    VideoServer videoServer;
public:
    VideoExtractor(){};
    ~VideoExtractor(){};
    virtual bool extract(VideoServer *server) = 0;
};

#endif // VIDEOEXTRACTOR_H
