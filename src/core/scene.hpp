#pragma once

#include <gvox/gvox.h>

#include <daxa/daxa.hpp>
#include <daxa/utils/task_graph.hpp>

struct VoxelScene {
    GvoxContainer main_container{};
    GvoxVoxelDesc voxel_desc{};

    VoxelScene();
    ~VoxelScene();

    VoxelScene(const VoxelScene &) = delete;
    VoxelScene(VoxelScene &&) = delete;
    auto operator=(const VoxelScene &) -> VoxelScene & = delete;
    auto operator=(VoxelScene &&) -> VoxelScene & = delete;
};
