#include <renderer/viewport.inl>

#if VIEWPORT_GENERATE

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main() {
}

#endif

#if VIEWPORT_RENDER

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main() {
    vec2 uv = vec2(gl_GlobalInvocationID.xy) / vec2(imageSize(daxa_image2D(target_image)));
    vec3 col = vec3(uv, 0);
    imageStore(daxa_image2D(target_image), ivec2(gl_GlobalInvocationID.xy), vec4(col, 1));
}

#endif
