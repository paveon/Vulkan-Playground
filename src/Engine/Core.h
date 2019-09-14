#ifndef VULKAN_CORE_H
#define VULKAN_CORE_H

#include <string>
#include <ctime>

inline const char* currentTime() {
   static char buf[80];
   time_t now = time(nullptr);
   tm tstruct = *localtime(&now);
   strftime(buf, sizeof(buf), "[%X]", &tstruct);
   return buf;
}

#endif //VULKAN_CORE_H
