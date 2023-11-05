#pragma once

#include <core/core.inl>

#if VIEWPORT_GENERATE || defined(__cplusplus)
DAXA_DECL_TASK_USES_BEGIN(ViewportGenerate, DAXA_UNIFORM_BUFFER_SLOT0)
DAXA_TASK_USE_BUFFER(voxels, daxa_RWBufferPtr(daxa_u32), COMPUTE_SHADER_READ_WRITE)
DAXA_DECL_TASK_USES_END()
#endif

#if VIEWPORT_RENDER || defined(__cplusplus)
DAXA_DECL_TASK_USES_BEGIN(ViewportRender, DAXA_UNIFORM_BUFFER_SLOT0)
DAXA_TASK_USE_BUFFER(voxels, daxa_BufferPtr(daxa_u32), COMPUTE_SHADER_READ)
DAXA_TASK_USE_IMAGE(target_image, REGULAR_2D, COMPUTE_SHADER_STORAGE_WRITE_ONLY)
DAXA_DECL_TASK_USES_END()
#endif

#if defined(__cplusplus)

#include <core/task_template.hpp>

namespace viewport {
    struct TaskCommon {
        static inline const std::string SHADER_FILE = "renderer/viewport.glsl";
        static auto get_defines() -> std::vector<daxa::ShaderDefine> { return {{"VIEWPORT", "1"}}; }
    };

    struct GenerateImpl : TaskCommon {
        static inline const std::string name = "viewport_generate";
        using Uses = ViewportGenerate;
        using PushConstant = void;
        struct Self {};
        static auto get_defines() -> std::vector<daxa::ShaderDefine> {
            auto result = TaskCommon::get_defines();
            result.push_back({"VIEWPORT_GENERATE", "1"});
            return result;
        }
        static void dispatch(daxa::TaskInterface const &, daxa::CommandRecorder &recorder, Self &self) {
            recorder.dispatch(1, 1, 1);
        }
    };

    struct RenderImpl : TaskCommon {
        static inline const std::string name = "viewport_render";
        using Uses = ViewportRender;
        using PushConstant = void;
        struct Self {
            daxa::TaskImageView target_image;
        };
        static auto get_defines() -> std::vector<daxa::ShaderDefine> {
            auto result = TaskCommon::get_defines();
            result.push_back({"VIEWPORT_RENDER", "1"});
            return result;
        }
        static void dispatch(daxa::TaskInterface const &ti, daxa::CommandRecorder &recorder, Self &self) {
            auto image_size = ti.get_device().info_image(ti.uses[self.target_image].image()).value().size;
            recorder.dispatch((image_size.x + 7) / 8, (image_size.y + 7) / 8, 1);
        }
    };

    using GenerateTaskState = TaskStateTemplate<GenerateImpl>;
    using GenerateTask = TaskTemplate<GenerateImpl>;

    using RenderTaskState = TaskStateTemplate<RenderImpl>;
    using RenderTask = TaskTemplate<RenderImpl>;
} // namespace viewport

#endif
