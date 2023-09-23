
WGPUAdapter RequestAdapter( WGPUInstance instance, WGPURequestAdapterOptions const * options )
{
    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData
    {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    // Callback called by wgpuInstanceRequestAdapter when the request returns
    // This is a C++ lambda function, but could be any function defined in the
    // global scope. It must be non-capturing (the brackets [] are empty) so
    // that it behaves like a regular C function pointer, which is what
    // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
    // is to convey what we want to capture through the pUserData pointer,
    // provided as the last argument of wgpuInstanceRequestAdapter and received
    // by the callback as its last argument.
    auto onAdapterRequestEnded = []( WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData )
    {
        UserData& userData = *reinterpret_cast<UserData*>( pUserData );

        if( status == WGPURequestAdapterStatus_Success )
            userData.adapter = adapter;
        else
            Log( "Could not get WebGPU adapter: %s", message );

        userData.requestEnded = true;
    };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter( instance, options, onAdapterRequestEnded, (void*)&userData );

    // In theory we should wait until onAdapterReady has been called, which
    // could take some time (what the 'await' keyword does in the JavaScript
    // code). In practice, we know that when the wgpuInstanceRequestAdapter()
    // function returns its callback has been called.
    ASSERT( userData.requestEnded, "Adapter request should have finished by now" );
    return userData.adapter;
}

WGPUDevice RequestDevice( WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor )
{
    struct UserData
    {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = []( WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData )
    {
        UserData& userData = *reinterpret_cast<UserData*>( pUserData );

        if( status == WGPURequestDeviceStatus_Success )
            userData.device = device;
        else
            Log( "Could not get WebGPU device: %s", message );

        userData.requestEnded = true;
    };

    wgpuAdapterRequestDevice( adapter, descriptor, onDeviceRequestEnded, (void*)&userData );

    ASSERT( userData.requestEnded, "Device request should have finished by now" );
    return userData.device;
}

void OnDeviceError( WGPUErrorType type, char const* message, void* pUserData )
{
    Log( "Uncaptured device ERROR: %d", type )
    if( message )
        Log( " '%s'", message );
};


WGPURenderPipeline CreatePipeline( Program const& program )
{
    // Load shaders
    Buffer<> shaderSource = Platform::ReadEntireFile( program.shaderPath, &globalAlloc, true );

    WGPUShaderModuleWGSLDescriptor shaderCodeDesc = {};
    shaderCodeDesc.chain.next                     = nullptr;
    shaderCodeDesc.chain.sType                    = WGPUSType_ShaderModuleWGSLDescriptor;
    shaderCodeDesc.code                           = (char const*)shaderSource.begin();
    WGPUShaderModuleDescriptor shaderDesc         = {};
#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount                          = 0;
    shaderDesc.hints                              = nullptr;
#endif
    shaderDesc.nextInChain                        = &shaderCodeDesc.chain;
    WGPUShaderModule shaderModule                 = wgpuDeviceCreateShaderModule( globalDevice, &shaderDesc );

    FREE( &globalAlloc, shaderSource.data );

    // Fragment, blend, color states
    WGPUFragmentState fragmentState  = {};
    fragmentState.module             = shaderModule;
    fragmentState.entryPoint         = "fs_main";
    fragmentState.constantCount      = 0;
    fragmentState.constants          = nullptr;
    WGPUBlendState blendState        = {};
    blendState.color.srcFactor       = WGPUBlendFactor_SrcAlpha;
    blendState.color.dstFactor       = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.color.operation       = WGPUBlendOperation_Add;
    blendState.alpha.srcFactor       = WGPUBlendFactor_Zero;
    blendState.alpha.dstFactor       = WGPUBlendFactor_One;
    blendState.alpha.operation       = WGPUBlendOperation_Add;
    WGPUColorTargetState colorTarget = {};
    colorTarget.format               = globalSwapChainFormat;
    colorTarget.blend                = &blendState;
    colorTarget.writeMask            = WGPUColorWriteMask_All;
    // We have only one target because our render pass has only one output color attachment.
    fragmentState.targetCount        = 1;
    fragmentState.targets            = &colorTarget;

    // Render pipeline
    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.nextInChain                  = nullptr;
    layoutDesc.bindGroupLayoutCount         = program.bindGroupLayout ? 1 : 0;
    layoutDesc.bindGroupLayouts             = program.bindGroupLayout ? &program.bindGroupLayout : nullptr;
    WGPUPipelineLayout pipelineLayout       = wgpuDeviceCreatePipelineLayout( globalDevice, &layoutDesc );

    WGPURenderPipelineDescriptor pipelineDesc       = {};
    pipelineDesc.nextInChain                        = nullptr;
    // TODO Only 1 buffer for now
    // TODO Assumes all attribs are in the same buffer
    pipelineDesc.vertex.bufferCount                 = program.vertexAttribs.size() ? 1 : 0;
    pipelineDesc.vertex.buffers                     = program.vertexAttribs.size() ? &program.vertexBufferLayout : nullptr;
    pipelineDesc.vertex.module                      = shaderModule;
    pipelineDesc.vertex.entryPoint                  = "vs_main";
    pipelineDesc.vertex.constantCount               = 0;
    pipelineDesc.vertex.constants                   = nullptr;
    // Each sequence of 3 vertices is considered as a triangle
    pipelineDesc.primitive.topology                 = (program.topology != -1) ? program.topology : WGPUPrimitiveTopology_TriangleStrip;
    // We'll see later how to specify the order in which vertices should be
    // connected. When not specified, vertices are considered sequentially.
    pipelineDesc.primitive.stripIndexFormat         = WGPUIndexFormat_Undefined;
    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    pipelineDesc.primitive.frontFace                = WGPUFrontFace_CCW;
    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipelineDesc.primitive.cullMode                 = WGPUCullMode_None;
    pipelineDesc.fragment                           = &fragmentState;
    pipelineDesc.depthStencil                       = nullptr;
    // Samples per pixel
    pipelineDesc.multisample.count                  = 1;
    // Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.mask                   = ~0u;
    // Default value as well (irrelevant for count = 1 anyways)
    pipelineDesc.multisample.alphaToCoverageEnabled = false;
    pipelineDesc.layout = pipelineLayout;

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline( globalDevice, &pipelineDesc );
    return pipeline;
}


bool SetCurrentProgram( Program& program )
{
    globalProgram = &program;

    if( program.initFunc )
        program.initFunc( &program, program.userdata );

    globalPipeline = CreatePipeline( program );
    // TODO Draw a pink screen when this is invalid
    return globalPipeline != nullptr;
}

void UpdateCurrentProgramInputs( f32 viewportWidth, f32 viewportHeight )
{
    if( globalProgram && globalProgram->updateFunc )
        globalProgram->updateFunc( globalProgram, globalProgram->userdata, viewportWidth, viewportHeight );
}

void ParseSwitchFile( char const* path );

bool OnShaderUpdated( char const* filename )
{
    char path[256];
    snprintf( path, sizeof(path), "%s/%s", ShadersDir, filename );

    bool result = false;
    if( strcmp( path, "src/shaders/switch.wgsl" ) == 0 )
    {
        ParseSwitchFile( path );
        result = true;
    }
    else if( globalProgram && strcmp( path, globalProgram->shaderPath ) == 0 )
    {
        // Recreate the pipeline
        // TODO Reinit?
        globalPipeline = CreatePipeline( *globalProgram );
        result = true;
    }

    return result;
}


bool Present( WGPUSwapChain swapChain )
{
    WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView( swapChain );
    if( !nextTexture )
    {
        // TODO Handle this?
        Log( "Cannot acquire next swap chain texture" );
        return false;
    }

    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain                  = nullptr;
    encoderDesc.label                        = "Command encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder( globalDevice, &encoderDesc );

    WGPURenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view                          = nextTexture;
    renderPassColorAttachment.resolveTarget                 = nullptr;
    renderPassColorAttachment.loadOp                        = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp                       = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue                    = ClearColor;

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain              = nullptr;
    renderPassDesc.colorAttachmentCount     = 1;
    renderPassDesc.colorAttachments         = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment   = nullptr;
    renderPassDesc.timestampWriteCount      = 0;
    renderPassDesc.timestampWrites          = nullptr;

    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass( encoder, &renderPassDesc );
    // Select which render pipeline to use
    wgpuRenderPassEncoderSetPipeline( renderPass, globalPipeline );

    if( globalProgram->bindGroup )
    {
        // Set binding group
        wgpuRenderPassEncoderSetBindGroup( renderPass, 0, globalProgram->bindGroup, 0, nullptr );
    }
    if( globalProgram->vertexBuffer )
    {
        // Set vertex buffer while encoding the render pass
        size_t bufferSize = globalProgram->elementCount * globalProgram->vertexBufferLayout.arrayStride;
        wgpuRenderPassEncoderSetVertexBuffer( renderPass, 0, globalProgram->vertexBuffer, 0, bufferSize );
        // Draw 1 vertex per point in the buffer
        wgpuRenderPassEncoderDraw( renderPass, globalProgram->elementCount, 1, 0, 0 );
    }
    else
    {
        // TODO Assume no vertex buffer means we just want a fullscreen quad
        wgpuRenderPassEncoderDraw( renderPass, 4, 1, 0, 0 );
    }

    wgpuRenderPassEncoderEnd( renderPass );

    wgpuTextureViewRelease( nextTexture );

    std::vector<WGPUCommandBuffer> commands;
    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain                 = nullptr;
    cmdBufferDescriptor.label                       = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish( encoder, &cmdBufferDescriptor );
    commands.push_back( command );
    // Submit
    wgpuQueueSubmit( globalQueue, commands.size(), commands.data() );

#ifdef WEBGPU_BACKEND_DAWN
    wgpuCommandEncoderRelease( encoder );
    wgpuCommandBufferRelease( command );
#endif

    wgpuSwapChainPresent( swapChain );
    return true;
}

WGPUBindGroupLayoutEntry DefaultBinding()
{
    WGPUBindGroupLayoutEntry binding;
    binding.nextInChain = nullptr;

    binding.buffer.nextInChain = nullptr;
    binding.buffer.type = WGPUBufferBindingType_Undefined;
    binding.buffer.hasDynamicOffset = false;

    binding.sampler.nextInChain = nullptr;
    binding.sampler.type = WGPUSamplerBindingType_Undefined;

    binding.storageTexture.nextInChain = nullptr;
    binding.storageTexture.access = WGPUStorageTextureAccess_Undefined;
    binding.storageTexture.format = WGPUTextureFormat_Undefined;
    binding.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    binding.texture.nextInChain = nullptr;
    binding.texture.multisampled = false;
    binding.texture.sampleType = WGPUTextureSampleType_Undefined;
    binding.texture.viewDimension = WGPUTextureViewDimension_Undefined;

    return binding;
}

// TODO Only one uniform buffer in one binding in one group supported rn
void InitUniformBuffer( Program* program, WGPUShaderStageFlags visibility, size_t size )
{
    WGPUBindGroupLayoutEntry bindingLayout = DefaultBinding();
    // The binding index as used in the @binding attribute in the shader
    bindingLayout.binding = 0;
    // The stage that needs to access this resource
    bindingLayout.visibility = visibility;
    bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayout.buffer.minBindingSize = size;

    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.nextInChain = nullptr;
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindingLayout;
    program->bindGroupLayout = wgpuDeviceCreateBindGroupLayout( globalDevice, &bindGroupLayoutDesc );
}

void WriteUniformBuffer( Program* program, void* data, size_t size )
{
    // Create uniform buffer
    WGPUBufferDescriptor uniformBufferDesc = {};
    uniformBufferDesc.nextInChain = nullptr;
    uniformBufferDesc.size = size;
    // Make sure to flag the buffer as BufferUsage::Uniform
    uniformBufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    uniformBufferDesc.mappedAtCreation = false;
    program->uniformBuffer = wgpuDeviceCreateBuffer( globalDevice, &uniformBufferDesc );

    wgpuQueueWriteBuffer( globalQueue, program->uniformBuffer, 0, data, size );

    // Create a binding
    WGPUBindGroupEntry binding = {};
    binding.nextInChain = nullptr;
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding.binding = 0;
    // The buffer it is actually bound to
    binding.buffer = program->uniformBuffer;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding.offset = 0;
    // And we specify again the size of the buffer.
    binding.size = uniformBufferDesc.size;

    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.nextInChain = nullptr;
    bindGroupDesc.layout = program->bindGroupLayout;
    // There must be as many bindings as declared in bindGroupLayoutDesc!
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &binding;
    program->bindGroup = wgpuDeviceCreateBindGroup( globalDevice, &bindGroupDesc );
}

