// https://gist.github.com/greggman/3c7b4729e0f0aedf49b6993e54050527

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


// Protean clouds by nimitz (twitter: @stormoid)
// https://www.shadertoy.com/view/3l23Rh
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
// Contact the author for other licensing options
/*
   Technical details:
   The main volume noise is generated from a deformed periodic grid, which can produce
   a large range of noise-like patterns at very cheap evalutation cost. Allowing for multiple
   fetches of volume gradient computation for improved lighting.
   To further accelerate marching, since the volume is smooth, more than half the density
   information isn't used to rendering or shading but only as an underlying volume	distance to 
   determine dynamic step size, by carefully selecting an equation	(polynomial for speed) to 
   step as a function of overall density (not necessarialy rendered) the visual results can be 
   the	same as a naive implementation with ~40% increase in rendering performance.
   Since the dynamic marching step size is even less uniform due to steps not being rendered at all
   the fog is evaluated as the difference of the fog integral at each rendered step.
 */

fn rot(a: f32) -> mat2x2<f32>
{
    var c = cos(a);
    var s = sin(a);
    return mat2x2<f32>(vec2<f32>(c,s),vec2<f32>(-s,c));
}

fn mag2(p: vec2<f32>) -> f32
{
    return dot(p,p);
}

fn linstep(mn: f32, mx: f32, x: f32) -> f32
{
    return clamp((x - mn)/(mx - mn), 0., 1.);
}

var<private> prm1: f32 = 0.;
var<private> bsMo: vec2<f32> = vec2<f32>(0.0,0.0);

fn disp(t: f32) -> vec2<f32>
{
    return vec2<f32>(sin(t*0.22)*1., cos(t*0.175)*1.)*2.;
}

fn map(p_: vec3<f32>) -> vec2<f32>
{
    var m3: mat3x3<f32> = mat3x3<f32>(
        vec3<f32>(0.33338, 0.56034, -0.71817), 
        vec3<f32>(-0.87887, 0.32651, -0.15323), 
        vec3<f32>(0.15162, 0.69596, 0.61339)
    )*1.93;

    var p = p_;
    var p2: vec3<f32> = p;
    p2 = vec3<f32>(p2.xy - disp(p.z), p2.z);
    p = vec3<f32>(p.xy * rot(sin(p.z+uniforms.iTime)*(0.1 + prm1*0.05) + uniforms.iTime*0.09), p.z);
    var cl = mag2(p2.xy);
    var d = 0.;
    p = p * .61;
    var z = 1.;
    var trk = 1.;
    var dspAmp = 0.1 + prm1*0.2;
    for(var i = 0; i < 5; i = i + 1)
    {
        p = p + sin(p.zxy*0.75*trk + uniforms.iTime*trk*.8)*dspAmp;
        d = d - abs(dot(cos(p), sin(p.yzx))*z);
        z = z * 0.57;
        trk = trk * 1.4;
        p = p*m3;
    }
    d = abs(d + prm1*3.)+ prm1*.3 - 2.5 + bsMo.y;
    return vec2<f32>(d + cl*.2 + 0.25, cl);
}

fn render(ro: vec3<f32>, rd: vec3<f32>, time: f32) -> vec4<f32>
{
    var rez = vec4<f32>(0.0, 0.0, 0.0, 0.0);
    var ldst = 8.;
    var lpos = vec3<f32>(disp(time + ldst)*0.5, time + ldst);
    var t = 1.5;
    var fogT = 0.;
    for(var i=0; i<130; i = i + 1)
    {
        if(rez.a > 0.99) {break;}
        var pos = ro + t*rd;
        var mpv = map(pos);
        var den = clamp( mpv.x - 0.3, 0.0, 1.0) * 1.12;
        var dn = clamp((mpv.x + 2.0), 0.0, 3.0);

        var col = vec4<f32>(0.0, 0.0, 0.0, 0.0);
        if (mpv.x > 0.6)
        {

            col = vec4<f32>(sin(vec3<f32>(5.,0.4,0.2) + mpv.y*0.1 +sin(pos.z*0.4)*0.5 + 1.8)*0.5 + 0.5,0.08);
            col = col * den*den*den;
            col = vec4<f32>(col.rgb * linstep(4.,-2.5, mpv.x)*2.3, col.a);
            var dif =  clamp((den - map(pos+.8).x)/9., 0.001, 1. );
            dif = dif + clamp((den - map(pos+.35).x)/2.5, 0.001, 1. );
            col = vec4<f32>(col.xyz * den*(vec3<f32>(0.005,.045,.075) + 1.5*vec3<f32>(0.033,0.07,0.03)*dif), col.w);
        }

        var fogC = exp(t*0.2 - 2.2);
        col = col + vec4<f32>(0.06,0.11,0.11, 0.1)*clamp(fogC-fogT, 0., 1.);
        fogT = fogC;
        rez = rez + col*(1. - rez.a);
        t = t + clamp(0.5 - dn*dn*.05, 0.09, 0.3);
    }
    return clamp(rez, vec4<f32>(0.0, 0.0, 0.0, 0.0), vec4<f32>(1.0, 1.0, 1.0, 1.0));
}

fn getsat(c: vec3<f32>) -> f32
{
    var mi = min(min(c.x, c.y), c.z);
    var ma = max(max(c.x, c.y), c.z);
    return (ma - mi)/(ma + 0.0000001);
}

//from my "Will it blend" shader (https://www.shadertoy.com/view/lsdGzN)
fn iLerp(a: vec3<f32>, b: vec3<f32>, x: f32) -> vec3<f32>
{
    var ic = mix(a, b, x) + vec3<f32>(0.000001, 0.,0.);
    var sd = abs(getsat(ic) - mix(getsat(a), getsat(b), x));
    var dir = normalize(vec3<f32>(2.*ic.x - ic.y - ic.z, 2.*ic.y - ic.x - ic.z, 2.*ic.z - ic.y - ic.x));
    var lgt = dot(vec3<f32>(1.0), ic);
    var ff = dot(dir, normalize(ic));
    ic = ic + 1.5*dir*sd*ff*lgt;
    return clamp(ic,vec3<f32>(0.,0.,0.),vec3<f32>(1.,1.,1.));
}


fn mainImage(fragCoord: vec2<f32>) -> vec4<f32>
{
    var q = fragCoord.xy/uniforms.iResolution.xy;
    var p = (fragCoord.xy - 0.5*uniforms.iResolution.xy)/uniforms.iResolution.y;
    /*bsMo = (uniforms.iMouse.xy - 0.5*uniforms.iResolution.xy)/uniforms.iResolution.y;*/

    var time = uniforms.iTime*3.;
    var ro = vec3<f32>(0.0,0.0,time);

    ro = ro + vec3<f32>(sin(uniforms.iTime)*0.5,sin(uniforms.iTime*1.)*0.,0.);

    var dspAmp = .85;
    ro = vec3<f32>(ro.xy + disp(ro.z)*dspAmp, ro.z);
    var tgtDst = 3.5;

    var tgt = normalize(ro - vec3<f32>(disp(time + tgtDst)*dspAmp, time + tgtDst));
    ro.x = ro.x - bsMo.x*2.;
    var rightdir = normalize(cross(tgt, vec3<f32>(0.,1.,0.)));
    var updir = normalize(cross(rightdir, tgt));
    rightdir = normalize(cross(updir, tgt));
    var rd=normalize((p.x*rightdir + p.y*updir)*1. - tgt);
    rd = vec3<f32>(rd.xy * rot(-disp(time + 3.5).x*0.2 + bsMo.x), rd.z);
    prm1 = smoothstep(-0.4, 0.4,sin(uniforms.iTime*0.3));
    var scn = render(ro, rd, time);

    var col = scn.rgb;
    col = iLerp(col.bgr, col.rgb, clamp(1.-prm1,0.05,1.));

    /*col = pow(col, vec3<f32>(.55,0.65,0.6))*vec3<f32>(1.,.97,.9);*/
    /*col = col * pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.12)*0.7+0.3; //Vign*/

    return vec4<f32>( col, 1.0 );
}

@fragment
fn fs_main( @builtin(position) fragCoord: vec4<f32> ) -> @location(0) vec4<f32>
{
    return mainImage(fragCoord.xy);
}

