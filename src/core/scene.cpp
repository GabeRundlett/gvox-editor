#include <core/scene.hpp>

#include <gvox/core.h>
#include <gvox/format.h>
#include <gvox/stream.h>

#include <gvox/containers/raw.h>

VoxelScene::VoxelScene() {
    {
        auto const attribs = std::array{
            GvoxAttribute{
                .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                .next = nullptr,
                .type = GVOX_ATTRIBUTE_TYPE_ALBEDO_PACKED,
                .format = GVOX_STANDARD_FORMAT_R8G8B8_SRGB,
            },
        };
        auto const create_info = GvoxVoxelDescCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
            .next = nullptr,
            .attribute_count = static_cast<uint32_t>(attribs.size()),
            .attributes = attribs.data(),
        };
        auto result = gvox_create_voxel_desc(&create_info, &voxel_desc);
    }

    {
        auto raw_container_conf = GvoxRawContainerConfig{
            .voxel_desc = voxel_desc,
        };
        auto const create_info = GvoxContainerCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO,
            .next = nullptr,
            .description = gvox_container_raw_description(),
            .cb_args = {
                .struct_type = GVOX_STRUCT_TYPE_CONTAINER_CREATE_CB_ARGS,
                .next = nullptr,
                .config = &raw_container_conf,
            },
        };
        auto result = gvox_create_container(&create_info, &main_container);
    }

    {
        auto voxel_data = uint32_t{};
        auto offset = GvoxOffset3D{0, 0, 0};
        auto extent = GvoxExtent3D{8, 8, 8};
        auto fill_info = GvoxFillInfo{
            .struct_type = GVOX_STRUCT_TYPE_FILL_INFO,
            .next = nullptr,
            .src_data = &voxel_data,
            .src_desc = voxel_desc,
            .dst = main_container,
            .range = {
                {3, &offset.x},
                {3, &extent.x},
            },
        };
        gvox_fill(&fill_info);

        voxel_data = 0x000000ff;
        offset = {1, 0, 0};
        extent = {7, 0, 0};
        gvox_fill(&fill_info);

        voxel_data = 0x0000ff00;
        offset = {0, 1, 0};
        extent = {0, 7, 0};
        gvox_fill(&fill_info);

        voxel_data = 0x00ff0000;
        offset = {0, 0, 1};
        extent = {0, 0, 7};
        gvox_fill(&fill_info);
    }
}

VoxelScene::~VoxelScene() {
    gvox_destroy_voxel_desc(voxel_desc);
    gvox_destroy_container(main_container);
}
