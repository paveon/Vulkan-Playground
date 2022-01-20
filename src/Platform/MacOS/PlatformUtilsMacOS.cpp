#if defined (__APPLE__)

#include <Engine/Utils/PlatformUtils.h>
#include <Engine/Application.h>
#include "DialogsMacOS.h"
#include <string>
#include <iostream>


/* MacOS UI code has to be run on main thread */
void FileDialogs::OpenFile(const char *filterName,
                           const std::vector<std::string> &filters,
                           const std::function<void(const std::string&)>& callback) {
   std::condition_variable cv;
   std::mutex cv_m;
   std::string path;

   Application::ExecuteOnMainThread([filters]() -> std::shared_ptr<void> {
      return GetFilePath(filters);
   }, [callback, &cv, &path](const std::shared_ptr<void> &payload) {
      if (payload) {
         path = *std::static_pointer_cast<std::string>(payload);
//         callback(*std::static_pointer_cast<std::string>(payload));
      }
      cv.notify_all();
   });

   std::unique_lock<std::mutex> lk(cv_m);
   cv.wait(lk);
   if (!path.empty())
      callback(path);
}

auto FileDialogs::SaveFile(const char *filterName, const char *filter) -> std::string {
   return "";
}

#endif