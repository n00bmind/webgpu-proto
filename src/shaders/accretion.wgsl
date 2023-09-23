struct VertexInput
{
    @location(0) position: vec3f,
    @location(1) velocity: vec3f,
    @location(2) color: vec4f,
};

struct VertexOutput
{
    @builtin(position) position: vec4f,
    @location(0) velocity: vec3f,
    // The location here does not refer to a vertex attribute, it just means
    // that this field must be handled by the rasterizer.
    // (It can also refer to another field of another struct that would be used
    // as input to the fragment shader.)
    @location(1) color: vec4f,
};

// Can use these in both VS & FS (as declared in bindingLayout.visibility)
struct Uniforms
{
    transform: mat4x4f,
    time: f32,
};
@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn vs_main( in: VertexInput ) -> VertexOutput
{
    var out: VertexOutput;
    /*out.position = vec4f( in.position, 1.0 );*/
    out.position = uniforms.transform * vec4f( in.position, 1.0 );
    out.velocity = in.velocity;
    out.color = in.color; // forward to the fragment shader
    return out;
}

@fragment
fn fs_main( in: VertexOutput ) -> @location(0) vec4f
{
    var offset = 0.9 * vec2f( cos( uniforms.time ), -sin( uniforms.time ) );

    return in.color - vec4f( offset, 0.0, 0.0 );
}
