#pragma once

#include "utils/functions.h"
#include "network/network.h"

class VideoExtractor
{
public:
    VideoExtractor(){};
    ~VideoExtractor(){};

    virtual QString extractLink(std::string link) = 0;
};


