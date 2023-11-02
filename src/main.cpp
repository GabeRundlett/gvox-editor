#include <gvox/gvox.h>
#include <daxa/daxa.hpp>
#include <fmt/format.h>

#include "ui/app_window.hpp"
#include "ui/app_ui.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

auto main() -> int {
    auto daxa_instance = daxa::create_instance({});
    auto device = daxa_instance.create_device({.name = "device"});

    auto app_ui = AppUi(device);

    auto on_update = [&]() {
        auto &app_window = app_ui.app_windows[0];

        auto swapchain_lock = std::lock_guard{*app_window.swapchain_mtx};
        auto const swapchain_image = app_window.swapchain.acquire_next_image();
        if (swapchain_image.is_empty()) {
            return;
        }

        auto const swapchain_image_full_slice = device.info_image_view(swapchain_image.default_view()).value().slice;
        auto recorder = device.create_command_recorder({.name = "my command recorder"});

        recorder.pipeline_barrier_image_transition({
            .dst_access = daxa::AccessConsts::TRANSFER_WRITE,
            .src_layout = daxa::ImageLayout::UNDEFINED,
            .dst_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .image_slice = swapchain_image_full_slice,
            .image_id = swapchain_image,
        });

        recorder.clear_image({
            .dst_image_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .clear_value = std::array<daxa::f32, 4>{0.0f, 0.0f, 0.0f, 1.0f},
            .dst_image = swapchain_image,
            .dst_slice = swapchain_image_full_slice,
        });

        recorder.pipeline_barrier_image_transition({
            .src_access = daxa::AccessConsts::TRANSFER_WRITE,
            .src_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .dst_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .image_slice = swapchain_image_full_slice,
            .image_id = swapchain_image,
        });

        app_ui.render(recorder, swapchain_image);

        recorder.pipeline_barrier_image_transition({
            .src_access = daxa::AccessConsts::TRANSFER_WRITE,
            .src_layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
            .dst_layout = daxa::ImageLayout::PRESENT_SRC,
            .image_slice = swapchain_image_full_slice,
            .image_id = swapchain_image,
        });

        auto executable_commands = recorder.complete_current_commands();
        recorder.~CommandRecorder();

        auto const &acquire_semaphore = app_window.swapchain.get_acquire_semaphore();
        auto const &present_semaphore = app_window.swapchain.get_present_semaphore();
        auto const &gpu_timeline = app_window.swapchain.get_gpu_timeline_semaphore();
        auto const cpu_timeline = app_window.swapchain.get_cpu_timeline_value();

        device.submit_commands(daxa::CommandSubmitInfo{
            .command_lists = std::array{executable_commands},
            .wait_binary_semaphores = std::array{acquire_semaphore},
            .signal_binary_semaphores = std::array{present_semaphore},
            .signal_timeline_semaphores = std::array{std::pair{gpu_timeline, cpu_timeline}},
        });
        device.present_frame({
            .wait_binary_semaphores = std::array{present_semaphore},
            .swapchain = app_window.swapchain,
        });

        device.collect_garbage();
    };

    app_ui.app_windows[0].on_resize = [&]() {
        on_update();
    };

    while (true) {
        app_ui.update();

        if (app_ui.should_close.load()) {
            break;
        }

        on_update();
    }

    app_ui.app_windows.clear();

    device.wait_idle();
    device.collect_garbage();
}
