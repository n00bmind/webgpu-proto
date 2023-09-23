
void InitFSQ( Program* program, void* userdata )
{
    program->topology = WGPUPrimitiveTopology_TriangleStrip; 
}
Program fsqProgram =
{
    "src/shaders/fsq_basic.wgsl",
    InitFSQ,
    nullptr,
};


struct ShadertoyUniforms
{
    v2 iResolution;
    f32 iTime;
    f32 _pad;
};

void InitStarfield( Program* program, void* userdata )
{
    program->topology = WGPUPrimitiveTopology_TriangleStrip; 

    // Create binding layout for a uniform
    InitUniformBuffer( program,
                       WGPUShaderStage_Fragment,
                       sizeof(ShadertoyUniforms) );
}
void UpdateStarfield( Program* program, void* userdata, f32 viewportWidth, f32 viewportHeight )
{
    ShadertoyUniforms uniforms;
    uniforms.iResolution = V2( viewportWidth, viewportHeight );
    uniforms.iTime = Platform::AppTimeSeconds();

    WriteUniformBuffer( program, &uniforms, sizeof(uniforms) );
}
Program starfieldProgram =
{
    "src/shaders/starfield.wgsl",
    InitStarfield,
    UpdateStarfield,
};


void InitFire( Program* program, void* userdata )
{
    program->topology = WGPUPrimitiveTopology_TriangleStrip; 

    // Create binding layout for a uniform
    InitUniformBuffer( program,
                       WGPUShaderStage_Fragment,
                       sizeof(ShadertoyUniforms) );
}
void UpdateFire( Program* program, void* userdata, f32 viewportWidth, f32 viewportHeight )
{
    ShadertoyUniforms uniforms;
    uniforms.iResolution = V2( viewportWidth, viewportHeight );
    uniforms.iTime = Platform::AppTimeSeconds();

    WriteUniformBuffer( program, &uniforms, sizeof(uniforms) );
}
Program fireProgram =
{
    "src/shaders/fire.wgsl",
    InitFire,
    UpdateFire,
};


void InitClouds( Program* program, void* userdata )
{
    program->topology = WGPUPrimitiveTopology_TriangleStrip; 

    // Create binding layout for a uniform
    InitUniformBuffer( program,
                       WGPUShaderStage_Fragment,
                       sizeof(ShadertoyUniforms) );
}
void UpdateClouds( Program* program, void* userdata, f32 viewportWidth, f32 viewportHeight )
{
    ShadertoyUniforms uniforms;
    uniforms.iResolution = V2( viewportWidth, viewportHeight );
    uniforms.iTime = Platform::AppTimeSeconds();

    WriteUniformBuffer( program, &uniforms, sizeof(uniforms) );
}
Program cloudsProgram =
{
    "src/shaders/clouds.wgsl",
    InitClouds,
    UpdateClouds,
};


struct AccretionVertex
{
    v3 position;
    v3 velocity;
    v4 color;
};

// NOTE Careful when arranging attributes here as anything in a uniform struct must comply with strict alignment requirements
// See https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/shader-uniforms/multiple-uniforms.html#memory-layout-constraints
struct AccretionUniforms
{
    m4 transform;
    f32 time;
    f32 _pad[15];
};
static_assert( sizeof(AccretionUniforms) % sizeof(m4) == 0 );

struct AccretionState
{
    f32 cameraFovYDeg;
};
AccretionState accretionState;

void InitAccretion( Program* program, void* userdata )
{
    // Some initial config
    AccretionState* state = (AccretionState*)userdata;
    state->cameraFovYDeg = 100;


    program->topology = WGPUPrimitiveTopology_PointList;

    // Describe layout for input vertex buffer
    WGPUVertexAttribute attribs[3];
    // position
    attribs[0].shaderLocation = 0;
    attribs[0].format = WGPUVertexFormat_Float32x3;
    attribs[0].offset = offsetof( AccretionVertex, position );
    // velocity
    attribs[1].shaderLocation = 1;
    attribs[1].format = WGPUVertexFormat_Float32x3;
    attribs[1].offset = offsetof( AccretionVertex, velocity );
    // color
    attribs[2].shaderLocation = 2;
    attribs[2].format = WGPUVertexFormat_Float32x4;
    attribs[2].offset = offsetof( AccretionVertex, color );

    program->vertexAttribs.insert( program->vertexAttribs.end(), attribs, attribs + ARRAYCOUNT(attribs) );

    program->vertexBufferLayout = {};
    //program->vertexBufferLayout.nextInChain = nullptr;     // Undefined?
    program->vertexBufferLayout.attributeCount = ARRAYCOUNT(attribs);
    program->vertexBufferLayout.attributes = program->vertexAttribs.data();
    program->vertexBufferLayout.arrayStride = sizeof(AccretionVertex);
    // One instance of AccretionVertex per vertex
    program->vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;


    // Create binding layout for a uniform
    InitUniformBuffer( program,
                       WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
                       sizeof(AccretionUniforms) );
}

void UpdateAccretion( Program* program, void* userdata, f32 viewportWidth, f32 viewportHeight )
{
    AccretionState* state = (AccretionState*)userdata;

    // Clear whatever was there from last pass
    if( program->vertexBuffer )
    {
        wgpuBufferDestroy( program->vertexBuffer );
        wgpuBufferRelease( program->vertexBuffer );
    }


    constexpr int MaxPoints = 1024;

    program->elementCount = MaxPoints;
    size_t bufferSize = program->elementCount * program->vertexBufferLayout.arrayStride;

    // Vertex buffer
    WGPUBufferDescriptor vertexBufferDesc = {};
    vertexBufferDesc.nextInChain = nullptr;
    vertexBufferDesc.label = "Point positions";
    vertexBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    vertexBufferDesc.size = bufferSize;
    vertexBufferDesc.mappedAtCreation = false;
    program->vertexBuffer = wgpuDeviceCreateBuffer( globalDevice, &vertexBufferDesc );

    // Create some CPU-side data buffer (3 floats per vertex)
    f32 currentTime = Platform::AppTimeSeconds();
    RandomStream random( currentTime );
    AccretionVertex pointData[ MaxPoints * 3 ] = {};
    for( AccretionVertex& p : pointData )
    {
        p.position = { random.GetFloat( -1, 1 ), random.GetFloat( -1, 1 ), random.GetFloat( -1, 1 ) };
        p.color = { 1, 0, 0, 1 };
    }

    // Copy this from pointData (RAM) to VRAM
    wgpuQueueWriteBuffer( globalQueue, program->vertexBuffer, 0, pointData, bufferSize );


    AccretionUniforms uniforms;
    //uniforms.transform = M4Perspective( viewportWidth / viewportHeight, state->cameraFovYDeg );
    uniforms.transform = M4Identity;
    uniforms.time = currentTime;

    WriteUniformBuffer( program, &uniforms, sizeof(uniforms) );
}

Program accretionProgram =
{
    "src/shaders/accretion.wgsl",
    InitAccretion,
    UpdateAccretion,
    &accretionState,
};
