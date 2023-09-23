#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
// TODO UGH
#include <vector>


// App files as a unity build
#include "win32.h"
#include "basic.h"
#include "platform.h"
#include "memory.h"
#include "math.h"
#include "math_types.h"
#include "program.h"

// Some globals
WGPUDevice globalDevice;
WGPUQueue globalQueue;
WGPURenderPipeline globalPipeline;
WGPUTextureFormat globalSwapChainFormat;
Program* globalProgram;

constexpr char const* ShadersDir = "src/shaders";
//WGPUColor ClearColor = WGPUColor{ 1.0, 0.0, 1.0, 1.0 };
WGPUColor ClearColor = WGPUColor{ 0.0, 0.0, 0.0, 1.0 };

#include "basic.cpp"
#include "utils.cpp"
#include "platform.cpp"
#include "wgpu.cpp"
#include "program.cpp"

Program* globalProgramList[] =
{
    &starfieldProgram,
    &fireProgram,
    &cloudsProgram,
};



constexpr int WindowWidth = 1024;
constexpr int WindowHeight = 768;

int main()
{
    char cwd[MAX_PATH];
    Platform::GetWorkingDirectory( cwd, sizeof(cwd) );

    Log( "Current directory: %s", cwd );

    if( !glfwInit() )
    {
        Log( "Could not initialize GLFW!" );
        return 1;
    }

    // Create WebGPU instance
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

    WGPUInstance instance = wgpuCreateInstance( &desc );
    if( !instance )
    {
        Log( "Could not initialize WebGPU!" );
        return 1;
    }

    Log( "WGPU instance: %p", instance );

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    // TODO 
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
    GLFWwindow* window = glfwCreateWindow( WindowWidth, WindowHeight, "WebGPU runtime", NULL, NULL );

    if( !window )
    {
        Log( "Could not open window!" );
        glfwTerminate();
        return 1;
    }

    WGPUSurface surface = glfwGetWGPUSurface( instance, window );

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain               = nullptr;
    adapterOpts.compatibleSurface         = surface;
    WGPUAdapter adapter = RequestAdapter( instance, &adapterOpts );

    Log( "WGPU adapter: %p", adapter );

    // Enumerate features
    std::vector<WGPUFeatureName> features;
    size_t featureCount = wgpuAdapterEnumerateFeatures( adapter, nullptr );

    features.resize( featureCount );
    wgpuAdapterEnumerateFeatures( adapter, features.data() );

    Log( "Adapter features:" );
    // TODO Check enum WGPUFeatureName defined in webgpu.h
    for( auto f : features )
        Log( " - %d", f );

    WGPUSupportedLimits supported = {};
    ZERO( supported );
    wgpuAdapterGetLimits( adapter, &supported );

    WGPURequiredLimits required = {};
    ZERO( required );
    // This must be set even if we do not use storage buffers for now
    required.limits.minStorageBufferOffsetAlignment = supported.limits.minStorageBufferOffsetAlignment;
    required.limits.minUniformBufferOffsetAlignment = supported.limits.minUniformBufferOffsetAlignment;
    // Maximum size of a buffer 
    required.limits.maxBufferSize = 1 * 1024 * 1024;  // NOTE 1 Mb
    // Maximum stride between 2 consecutive vertices in the vertex buffer
    required.limits.maxVertexBufferArrayStride = 64; // NOTE 64 bytes
    required.limits.maxVertexBuffers = 1;
    required.limits.maxVertexAttributes = 4;
    required.limits.maxInterStageShaderComponents = 8;
    // We use at most 1 bind group for now
    required.limits.maxBindGroups = 1;
    // We use at most 1 uniform buffer per stage
    required.limits.maxUniformBuffersPerShaderStage = 1;
    // Uniform structs have a size of maximum 16 float
    //required.limits.maxUniformBufferBindingSize = 16 * sizeof(f32);
    required.limits.maxUniformBufferBindingSize = 16 * 2 * sizeof(f32);

    WGPUDeviceDescriptor deviceDesc     = {};
    deviceDesc.nextInChain              = nullptr;
    deviceDesc.label                    = "Main Device"; 
    deviceDesc.requiredFeaturesCount    = 0;        // we do not require any specific feature
    deviceDesc.requiredLimits           = &required;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label       = "The default queue";
    globalDevice = RequestDevice( adapter, &deviceDesc );

    Log( "WGPU device: %p", globalDevice );

    // Set a debug callback
    wgpuDeviceSetUncapturedErrorCallback( globalDevice, OnDeviceError, nullptr );

    // Command queue
    globalQueue = wgpuDeviceGetQueue( globalDevice );
    //auto onQueueWorkDone = []( WGPUQueueWorkDoneStatus status, void* pUserData )
    //{
        //Log( "Queued work finished with status: %d", status );
    //};
    //wgpuQueueOnSubmittedWorkDone( globalQueue, onQueueWorkDone, nullptr );

    // Swap chain
#ifdef WEBGPU_BACKEND_DAWN
    globalSwapChainFormat = WGPUTextureFormat_BGRA8Unorm;
#else
    globalSwapChainFormat = wgpuSurfaceGetPreferredFormat( surface, adapter );
#endif
    WGPUSwapChainDescriptor swapChainDesc = {};
    swapChainDesc.nextInChain             = nullptr;
    swapChainDesc.width                   = WindowWidth;
    swapChainDesc.height                  = WindowHeight;
    swapChainDesc.format                  = globalSwapChainFormat;
    swapChainDesc.usage                   = WGPUTextureUsage_RenderAttachment;
    swapChainDesc.presentMode             = WGPUPresentMode_Fifo;

    WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain( globalDevice, surface, &swapChainDesc );
    Log( "Swapchain created successfully" );


    // Set the program that we'll use
    SetCurrentProgram( cloudsProgram );

    // Start listening for directory changes
    ShaderUpdateListener listener = {};
    Platform::SetupShaderUpdateListener( ShadersDir, OnShaderUpdated, &listener );

    //  Main loop
    bool readyToPresent = true;
    while( !glfwWindowShouldClose( window ) )
    {
        // Check if the current shader was updated
        if( Platform::CheckShaderUpdates( &listener ) )
            // Re-try presenting if we had failed previously
            readyToPresent = true;

        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();

        if( readyToPresent )
        {
            // TODO Support resizing
            UpdateCurrentProgramInputs( WindowWidth, WindowHeight );
            readyToPresent = Present( swapChain );
        }
    }


    wgpuSwapChainRelease( swapChain );
    wgpuDeviceRelease( globalDevice );
    wgpuAdapterRelease( adapter );
    wgpuSurfaceRelease( surface );
    wgpuInstanceRelease( instance );
    glfwDestroyWindow( window );

    return 0;
}

void ParseSwitchFile( char const* filepath )
{
    Buffer<> shaderSource = Platform::ReadEntireFile( filepath, &globalAlloc, true );

    // Parse lines
    char path[256];
    char const* str = (char const*)shaderSource.data;
    for( ;; )
    {
        char const* lineEnd = strpbrk( str, "\r\n" );
        if( lineEnd == nullptr )
            break;

        // Skip commented lines
        if( strstr( str, "//" ) == str )
        {
            while( lineEnd && isspace( *lineEnd ) )
                lineEnd++;

            str = lineEnd;
            continue;
        }

        // Load whatever the line says
        snprintf( path, sizeof(path), "%s/%.*s", ShadersDir, (int)(lineEnd - str), str );
        break;
    }

    FREE( &globalAlloc, shaderSource.data );

    for( Program* p : globalProgramList )
    {
        if( strcmp( path, p->shaderPath ) == 0 )
        {
            SetCurrentProgram( *p );
            break;
        }
    }
}
