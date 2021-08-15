#ifndef GAME_ENGINE_PLATFORM_UTILS_H
#define GAME_ENGINE_PLATFORM_UTILS_H

#include <string>

class FileDialogs {
public:
    static auto OpenFile(const char* filterName, const char *filter) -> std::string;

    static auto SaveFile(const char* filterName, const char *filter) -> std::string;
};


#endif //GAME_ENGINE_PLATFORM_UTILS_H
