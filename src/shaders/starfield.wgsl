// https://www.shadertoy.com/view/dtlSRl

struct ShadertoyUniforms
{
    iResolution: vec2f,
    iTime: f32,
};
@group(0) @binding(0) var<uniform> uniforms: ShadertoyUniforms;


// Ideally we'd like this to be constant, but this errors out and points to a github issue in wgpu-native
//const positions = array(
var<private> positions: array<vec2f,4> = array<vec2f,4>(
    vec2f(-1.0, -1.0),
    vec2f( 1.0, -1.0),
    vec2f(-1.0,  1.0),
    vec2f( 1.0,  1.0)
);

@vertex
fn vs_main( @builtin(vertex_index) in_vertex_index: u32 ) -> @builtin(position) vec4f
{
    // Emit hardcoded positions for the 4 corners of the window
    // Invoke this with a WGPUPrimitiveTopology_TriangleStrip call (and a count of 4)
    return vec4f( positions[in_vertex_index], 0.0, 1.0 );
}


const NLAYERS = 128.0;

fn hash( p_in: vec2f) -> f32
{
    var p = p_in;
    p = fract( p * vec2f( sin(123.124), sin(928.0123) ) );
    p += dot( p,p+154.23 );
    return fract( p.x*p.y );
}

fn draw_star( uv: vec2f, intensity: f32 ) -> vec3f
{
    var d = length(uv);
    var col = vec3f(0.0);
    col += 0.3/pow(d,2.0);
    col.b *= 4.0;
    col *= intensity;
    col *= smoothstep(0.5, 0.2, d);
    return col;
}

fn star_field( uv: vec2f, intensity: f32 ) -> vec3f
{
    var gv = fract(uv) - 0.5;
    var id = floor(uv);

    var col = vec3f(0.0);

    for( var y: f32 =-1.0; y <= 1.0; y += 1.0 )
    {
        for( var x: f32 =-1.0;x<=1.0;x += 1.0 )
        {
            var offs = vec2f( x, y );
            var n = hash(id+offs);

            col += draw_star( gv - offs - vec2f( n - 0.5, fract( n*34.0 ) ) + 0.5, intensity );

        }
    }
    return col;
}

@fragment
fn fs_main( @builtin(position) fragCoord: vec4f ) -> @location(0) vec4f
{
   var uv: vec2f = (fragCoord.xy - 0.5*uniforms.iResolution.xy)/uniforms.iResolution.y;
    
   var col: vec3f = vec3f(0.0);
   for( var i=0.0 ; i<1.0 ; i+= 1.0/NLAYERS )
   {
       var t = uniforms.iTime*0.1;
       var depth = fract( i+t );
       var scale = mix( 10., 0.1, depth );
       col += star_field( uv*scale+i*4000.0, pow( i*.001, 1.0+i*0.5 )  );
   }
    
   return vec4f( col,1.0 );
}

