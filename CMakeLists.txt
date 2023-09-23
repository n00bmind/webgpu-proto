
cmake_minimum_required(VERSION 3.0...3.25)
project(
    WebGPURuntime # name of the project, which will also be the name of the visual studio solution if you use it
    VERSION 0.0.0 # any version number
    LANGUAGES CXX C # programming languages used by the project
)

add_executable(App src/main.cpp)

set_target_properties(App PROPERTIES
    CXX_STANDARD 17
    #COMPILE_WARNING_AS_ERROR ON
    # Dawn specific stuff to force it to immediately invoke our error callback as soon as an error happens
    VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
)
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-DCONFIG_DEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-DCONFIG_RELEASE=1")

add_subdirectory(3rdparty/glfw)
add_subdirectory(3rdparty/WebGPU-distribution-main)
add_subdirectory(3rdparty/glfw3webgpu)

target_link_libraries(App PRIVATE glfw webgpu glfw3webgpu)
# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
target_copy_webgpu_binaries(App)
