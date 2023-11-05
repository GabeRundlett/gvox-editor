#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <daxa/utils/task_graph.hpp>
#include <fmt/format.h>

#include <renderer/viewport.hpp>
#include <ui/app_ui.hpp>
#include <core/scene.hpp>

struct VoxelApp {
    daxa::Instance daxa_instance;
    daxa::Device daxa_device;
    daxa::PipelineManager pipeline_manager;
    VoxelScene scene;
    Viewport viewport;
    AppUi ui;
    daxa::TaskGraph main_task_graph;
    daxa::TaskImage task_swapchain_image;

    VoxelApp();
    ~VoxelApp();

    VoxelApp(const VoxelApp &) = delete;
    VoxelApp(VoxelApp &&) = delete;
    auto operator=(const VoxelApp &) -> VoxelApp & = delete;
    auto operator=(VoxelApp &&) -> VoxelApp & = delete;

    void update();
    auto should_close() -> bool;
    void render();
    auto record_main_task_graph() -> daxa::TaskGraph;
};

auto main() -> int {
    auto app = VoxelApp();
    while (true) {
        app.update();
        if (app.should_close()) {
            break;
        }
        app.render();
    }
}

VoxelApp::VoxelApp()
    : daxa_instance{daxa::create_instance({})},
      daxa_device{daxa_instance.create_device({.name = "device"})},
      pipeline_manager{[this]() {
          auto result = daxa::PipelineManager({
              .device = daxa_device,
              .shader_compile_options = {
                  .root_paths = {DAXA_SHADER_INCLUDE_DIR, "src"},
                  .language = daxa::ShaderLanguage::GLSL,
                  .enable_debug_info = true,
              },
              .register_null_pipelines_when_first_compile_fails = true,
              .name = "pipeline_manager",
          });
          return result;
      }()},
      viewport{pipeline_manager},
      ui{daxa_device},
      task_swapchain_image{daxa::TaskImageInfo{.swapchain_image = true}} {
    ui.app_windows[0].on_resize = [&]() {
        main_task_graph = record_main_task_graph();
        render();
    };
    main_task_graph = record_main_task_graph();
}

VoxelApp::~VoxelApp() {
    ui.app_windows.clear();
    daxa_device.wait_idle();
    daxa_device.collect_garbage();
}

void VoxelApp::update() {
    ui.update();
}

void VoxelApp::render() {
    auto &app_window = ui.app_windows[0];
    auto const swapchain_image = app_window.swapchain.acquire_next_image();
    if (swapchain_image.is_empty()) {
        return;
    }
    task_swapchain_image.set_images({.images = {&swapchain_image, 1}});
    main_task_graph.execute({});
    daxa_device.collect_garbage();
}

auto VoxelApp::should_close() -> bool {
    return ui.should_close.load();
}

auto VoxelApp::record_main_task_graph() -> daxa::TaskGraph {
    auto &app_window = ui.app_windows[0];
    auto task_graph = daxa::TaskGraph(daxa::TaskGraphInfo{
        .device = daxa_device,
        .swapchain = app_window.swapchain,
        .name = "main_tg",
    });
    task_graph.use_persistent_image(task_swapchain_image);
    task_graph.add_task({
        .uses = {
            daxa::TaskImageUse<daxa::TaskImageAccess::TRANSFER_WRITE, daxa::ImageViewType::REGULAR_2D>{task_swapchain_image},
        },
        .task = [this](daxa::TaskInterface task_runtime) {
            auto &recorder = task_runtime.get_recorder();
            auto swapchain_image = task_runtime.uses[task_swapchain_image].image();
            auto swapchain_image_full_slice = daxa_device.info_image_view(swapchain_image.default_view()).value().slice;
            recorder.clear_image({
                .dst_image_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .clear_value = std::array<daxa::f32, 4>{0.2f, 0.1f, 0.4f, 1.0f},
                .dst_image = swapchain_image,
                .dst_slice = swapchain_image_full_slice,
            });
        },
        .name = "clear screen",
    });
    auto viewport_render_image = task_graph.create_transient_image({
        .format = daxa::Format::R16G16B16A16_SFLOAT,
        .size = {static_cast<uint32_t>(app_window.size.x), static_cast<uint32_t>(app_window.size.y), 1},
        .name = "viewport_render_image",
    });
    viewport.render(task_graph, viewport_render_image);

    task_graph.add_task({
        .uses = {
            daxa::TaskImageUse<daxa::TaskImageAccess::TRANSFER_READ>{viewport_render_image},
            daxa::TaskImageUse<daxa::TaskImageAccess::TRANSFER_WRITE>{task_swapchain_image},
        },
        .task = [viewport_render_image, this](daxa::TaskInterface const &ti) {
            auto &recorder = ti.get_recorder();
            auto image_size = ti.get_device().info_image(ti.uses[viewport_render_image].image()).value().size;
            recorder.blit_image_to_image({
                .src_image = ti.uses[viewport_render_image].image(),
                .src_image_layout = ti.uses[task_swapchain_image].layout(),
                .dst_image = ti.uses[task_swapchain_image].image(),
                .dst_image_layout = ti.uses[task_swapchain_image].layout(),
                .src_offsets = {{{0, 0, 0}, {static_cast<int32_t>(image_size.x), static_cast<int32_t>(image_size.y), 1}}},
                .dst_offsets = {{{0, 0, 0}, {static_cast<int32_t>(image_size.x), static_cast<int32_t>(image_size.y), 1}}},
                .filter = daxa::Filter::LINEAR,
            });
        },
        .name = "blit_image_to_image",
    });

    task_graph.add_task({
        .uses = {
            daxa::TaskImageUse<daxa::TaskImageAccess::COLOR_ATTACHMENT, daxa::ImageViewType::REGULAR_2D>{task_swapchain_image},
        },
        .task = [this](daxa::TaskInterface task_runtime) {
            auto &recorder = task_runtime.get_recorder();
            ui.render(recorder, task_runtime.uses[task_swapchain_image].image());
        },
        .name = "ui draw",
    });
    task_graph.submit({});
    task_graph.present({});
    task_graph.complete({});
    return task_graph;
}
