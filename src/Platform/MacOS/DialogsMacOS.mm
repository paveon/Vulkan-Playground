#if defined (__APPLE__)

#include "DialogsMacOS.h"
#include <Cocoa/Cocoa.h>
#include <Engine/Utils/PlatformUtils.h>
#include <Engine/Application.h>
#include <string>
#include <iostream>
#include <algorithm>

auto GetFilePath(const std::vector<std::string> &fileTypes) -> std::shared_ptr<std::string> {
   NSArray *URLs = nullptr;
   NSOpenPanel *panel = [NSOpenPanel openPanel];
//   NSArray* allowedTypes = [NSArray arrayWithObjects:@"hdr",nil];
//   [NSString stringWithUTF8String:mystring.c_str()]
   std::vector<NSString*> allowedTypes;
   std::transform(fileTypes.begin(), fileTypes.end(), std::back_inserter(allowedTypes),
                  [](const std::string& type) -> NSString* {
      return [NSString stringWithUTF8String:type.c_str()];
   });
   NSArray *nsAllowedTypes = [NSArray arrayWithObjects:allowedTypes.data() count:allowedTypes.size()];
//   UTType* types = @[UTType @".hdr"];

   [panel setAllowsMultipleSelection:NO];
   [panel setCanChooseDirectories:NO];
   [panel setResolvesAliases:YES];
//   [panel setAllowedContentTypes:allowedTypes];
   [panel setAllowedFileTypes:nsAllowedTypes];
   [panel setTreatsFilePackagesAsDirectories:YES];
   [panel setMessage:@"Please choose an image file."];

   std::shared_ptr<std::string> path;
   if ([panel runModal] == NSModalResponseOK) {
      URLs = [panel URLs];
      NSUInteger count = [URLs count];
      path = std::make_shared<std::string>([[URLs[0] path] UTF8String]);
   }
   [panel close];
   return path;
}

#endif