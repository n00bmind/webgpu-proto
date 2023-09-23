#pragma once

struct Program;
using InitProgramFunc = void( Program*, void* );
using UpdateInputFunc = void( Program*, void*, f32, f32 );

struct Program
{
    // Program description (define these)
    char const* const shaderPath = nullptr;         // NOTE Use forward slashes!
    InitProgramFunc* const initFunc = nullptr;
    UpdateInputFunc* const updateFunc = nullptr;
    void* userdata = nullptr;

    // Runtime state
    WGPUPrimitiveTopology topology = (WGPUPrimitiveTopology)-1;
    std::vector<WGPUVertexAttribute> vertexAttribs;

    WGPUBindGroupLayout bindGroupLayout = {};
    WGPUBindGroup bindGroup = {};
    WGPUBuffer uniformBuffer = {};
    WGPUVertexBufferLayout vertexBufferLayout = {};
    WGPUBuffer vertexBuffer = {};
    int elementCount = 0;       // How many elements in the vertex buffer
};

