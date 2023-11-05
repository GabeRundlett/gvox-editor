#include <renderer/viewport.hpp>

Viewport::Viewport(daxa::PipelineManager &pipeline_manager)
    : generate_task_state(pipeline_manager),
      render_task_state(pipeline_manager) {}

void Viewport::render(daxa::TaskGraph &task_graph, daxa::TaskImageView target_image) {
    auto voxels_buffer = task_graph.create_transient_buffer({
        .size = static_cast<uint32_t>(sizeof(uint32_t) * 8 * 8 * 8),
        .name = "voxels",
    });
    task_graph.add_task(viewport::GenerateTask{
        {
            .uses = {
                .voxels = voxels_buffer,
            },
        },
        &generate_task_state,
        {},
    });
    task_graph.add_task(viewport::RenderTask{
        {
            .uses = {
                .voxels = voxels_buffer,
                .target_image = target_image,
            },
        },
        &render_task_state,
        {
            .target_image = target_image,
        },
    });
}
