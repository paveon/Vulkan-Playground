#include "RenderCommand.h"

std::unique_ptr<RendererAPI> RenderCommand::s_API = nullptr;


//static RenderCommand::PayloadInfo s_CommandInfo[RenderCommand::TYPE_MAX_ENUM] = {
//        {RenderCommand::SET_CLEAR_COLOR, sizeof(math::vec4)},
//        {RenderCommand::CLEAR, 0},
//        {RenderCommand::DRAW_INDEXED, sizeof(DrawIndexedPayload)},
//};