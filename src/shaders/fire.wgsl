//https://www.shadertoy.com/view/MdKfDh

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


const fireMovement        = vec2f(-0.09, -0.5);
const distortionMovement  = vec2f(-0.05, -0.3);
const normalStrength      = 50.0;
const distortionStrength  = 0.1;
const gradientPower         = 8.;
const gradientIntensity     = 7.;


/** NOISE **/
fn rand( co: vec2f ) -> f32
{
    return fract(sin(dot(co.xy ,vec2f(12.9898,78.233))) * 43758.5453);
}

fn hash( p_in: vec2f  ) -> vec2f
{
    var p = p_in;
    p = vec2f( dot(p,vec2f(127.1,311.7)),
               dot(p,vec2f(269.5,183.3)) );

    return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

fn noise( p: vec2f  ) -> f32
{
    let K1 = 0.366025404; // (sqrt(3)-1)/2;
    let K2 = 0.211324865; // (3-sqrt(3))/6;

    let i = floor( p + (p.x+p.y)*K1 );

    let a = p - i + (i.x+i.y)*K2;
    let o = step(a.yx,a.xy);    
    let b = a - o + K2;
    let c = a - 1.0 + 2.0*K2;

    let h = max( vec3f(0.5) - vec3f(dot(a,a), dot(b,b), dot(c,c) ), vec3f(0.0) );

    let n = h*h*h*h*vec3f( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));

    return dot( n, vec3f(70.0) );
}

fn fbm( p_in: vec2f  ) -> f32
{
    var p = p_in;
    var f = 0.0;
    let m = mat2x2f( 1.6,  1.2, -1.2,  1.6 );
    f  = 0.5000*noise(p); p = m*p;
    f += 0.2500*noise(p); p = m*p;
    f += 0.1250*noise(p); p = m*p;
    f += 0.0625*noise(p); p = m*p;
    f = 0.5 + 0.5 * f;
    return f;
}

/** DISTORTION **/
fn bumpMap( uv: vec2f ) -> vec3f
{ 
    let s = 1. / uniforms.iResolution.xy;
    let p =  fbm(uv);
    let h1 = fbm(uv + s * vec2f(1., 0.));
    let v1 = fbm(uv + s * vec2f(0., 1.));

    let xy = (p - vec2f(h1, v1)) * normalStrength;
    return vec3f(xy + .5, 1.);
}

// #define DEBUG_NORMAL

/** MAIN **/
@fragment
fn fs_main( @builtin(position) fragCoord: vec4f ) -> @location(0) vec4f
{
    let fragPos = vec2f( fragCoord.x, uniforms.iResolution.y - fragCoord.y );
    var uv = fragPos.xy/uniforms.iResolution.xy;

    let timeScale: f32 = uniforms.iTime * .5;

    let normal = bumpMap(uv * vec2f(1.0, 0.3) + distortionMovement * timeScale);

/*#ifdef DEBUG_NORMAL*/
    /*fragColor = vec4f(normal, 1.0);*/
    /*return;*/
/*#endif*/

    let displacement = clamp((normal.xy - .5) * distortionStrength, vec2f(-1.), vec2f(1.));
    uv += displacement; 

    let uvT = (uv * vec2f(1.0, 0.5)) + timeScale * fireMovement;
    let n = pow(fbm(8.0 * uvT), 1.0);    

    let gradient = pow(1.0 - uv.y, gradientPower) * gradientIntensity;
    let finalNoise = n * gradient;

    let color = finalNoise * vec3f(2.*n, 2.*n*n*n, n*n*n*n);
    return vec4f(color, 1.);
}
