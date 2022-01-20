#ifndef GAME_ENGINE_DIALOGSMACOS_H
#define GAME_ENGINE_DIALOGSMACOS_H

#if defined (__APPLE__)
#include <memory>

auto GetFilePath(const std::vector<std::string> &fileTypes) -> std::shared_ptr<std::string>;
#endif

#endif //GAME_ENGINE_DIALOGSMACOS_H
