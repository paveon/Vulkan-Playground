#ifndef GAME_ENGINE_PLATFORM_UTILS_H
#define GAME_ENGINE_PLATFORM_UTILS_H

#include <string>

class FileDialogs {
public:
    static void OpenFile(const char* filterName,
                         const std::vector<std::string> &filters,
                         const std::function<void(const std::string&)>& callback);

    static auto SaveFile(const char* filterName, const char *filter) -> std::string;
};


#endif //GAME_ENGINE_PLATFORM_UTILS_H
