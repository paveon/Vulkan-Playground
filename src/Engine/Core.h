#ifndef VULKAN_CORE_H
#define VULKAN_CORE_H

#include <string>
#include <ctime>
#include <chrono>

//inline auto currentTime() -> const char* {
//   static char buf[80];
//   time_t now = time(nullptr);
//   tm tstruct = *localtime(&now);
//   strftime(buf, sizeof(buf), "[%X]", &tstruct);
//   return buf;
//}


inline auto currentTime() -> const char* {
    static char buf[11] = {0};
    std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm tstruct = *localtime(&time);
    strftime(buf, sizeof(buf) / sizeof(char), "[%X]", &tstruct);
    return buf;
}

inline auto Log() -> std::ostream& {
   return std::cout << currentTime();
}


#endif //VULKAN_CORE_H
