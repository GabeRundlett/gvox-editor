#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_graph.hpp>
#include <daxa/utils/pipeline_manager.hpp>

#include <renderer/viewport.inl>

struct Viewport {
    viewport::GenerateTaskState generate_task_state;
    viewport::RenderTaskState render_task_state;

    explicit Viewport(daxa::PipelineManager &pipeline_manager);
    ~Viewport() = default;

    Viewport(const Viewport &) = delete;
    Viewport(Viewport &&) = delete;
    auto operator=(const Viewport &) -> Viewport & = delete;
    auto operator=(Viewport &&) -> Viewport & = delete;

    void render(daxa::TaskGraph &task_graph, daxa::TaskImageView target_image);
};
