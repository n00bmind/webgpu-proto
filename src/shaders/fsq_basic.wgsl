
// Ideally we'd like this to be constant, but this errors out and points to a github issue in wgpu-native
//const positions = array(
var<private> positions: array<vec2f,4> = array<vec2f,4>(
    vec2f(-1.0, -1.0),
    vec2f( 1.0, -1.0),
    vec2f(-1.0,  1.0),
    vec2f( 1.0,  1.0)
);

@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f
{
    // Emit hardcoded positions for the 4 corners of the window
    // Invoke this with a WGPUPrimitiveTopology_TriangleStrip call (and a count of 4)
    return vec4f( positions[in_vertex_index], 0.0, 1.0 );
}

@fragment
fn fs_main() -> @location(0) vec4f
{
    return vec4f(0.0, 0.0, 0.0, 1.0);
}
