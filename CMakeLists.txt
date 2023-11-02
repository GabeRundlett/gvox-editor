cmake_minimum_required(VERSION 3.21)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/deps.cmake")

project(gvox-editor VERSION 0.1.0)

add_executable(${PROJECT_NAME}
    "src/main.cpp"
    "src/ui/app_window.cpp"
    "src/ui/app_ui.cpp"
    "src/ui/rml/render_daxa.cpp"
    "src/ui/rml/system_glfw.cpp"
)

find_package(gvox CONFIG REQUIRED)
find_package(daxa CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(RmlUi CONFIG REQUIRED)
find_package(Stb REQUIRED)

target_link_libraries(${PROJECT_NAME}
PRIVATE
    gvox::gvox
    daxa::daxa
    glfw
    RmlCore RmlDebugger
)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${Stb_INCLUDE_DIR}
)
