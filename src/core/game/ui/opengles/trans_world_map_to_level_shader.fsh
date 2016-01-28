//#ifdef GL_ES
//precision highp float;
//#endif
//
//uniform sampler2D u_from;
//uniform sampler2D u_to;
//uniform float u_progress;
//
//varying vec2 v_TextureCoordinates;
//
//vec2 zoom(vec2 uv, float amount)
//{
//    return 0.5 + ((uv - 0.5) * amount);
//}
//
//void main() {
//    vec2 uv = v_TextureCoordinates;
//    vec2 r =  2.0 * vec2(uv.xy - 0.5 * 1.0) / 1.0;
//    float pro = u_progress / 0.8;
//    float z = (pro) * 0.2;
//    float t = 0.0;
//    
//    if (pro > 1.0)
//    {
//        z = 0.2 + (pro - 1.0) * 5.;
//        t = clamp((u_progress - 0.8) / 0.07,0.0,1.0);
//    }
//    
//    if (length(r) < 0.5+z)
//    {
//        //uv = zoom(uv, 0.9 - 0.1 * pro);
//    }
//    else if (length(r) < 0.8+z*1.5)
//    {
//        uv = zoom(uv, 1.0 - 0.15 * pro);
//        t = t * 0.5;
//    }
//    else if (length(r) < 1.2+z*2.5)
//    {
//        uv = zoom(uv, 1.0 - 0.2 * pro);
//        t = t * 0.2;
//    }
//    else
//    {
//        uv = zoom(uv, 1.0 - 0.25 * pro);
//    }
//    
//    gl_FragColor = mix(texture2D(u_from, uv), texture2D(u_to, uv), t);
//}

#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D u_from;
uniform sampler2D u_to;
uniform float u_progress;

varying vec2 v_TextureCoordinates;

highp float random(vec2 co)
{
    highp float a = 12.9898;
    highp float b = 78.233;
    highp float c = 43758.5453;
    highp float dt= dot(co.xy ,vec2(a,b));
    highp float sn= mod(dt,3.14);
    
    return fract(sin(sn) * c);
}

float voronoi(in vec2 x)
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    float res = 8.0;
    for (float j=-1.; j<=1.; j++)
    {
        for (float i=-1.; i<=1.; i++)
        {
            vec2  b = vec2(i, j);
            vec2  r = b - f + random(p + b);
            float d = dot(r, r);
            res = min(res, d);
        }
    }
    
    return sqrt(res);
}

vec2 displace(vec4 tex, vec2 texCoord, float dotDepth, float textureDepth, float strength)
{
    float b = voronoi(.003 * texCoord + 2.0);
    float g = voronoi(0.2 * texCoord);
    float r = voronoi(texCoord - 1.0);
    vec4 dt = tex * 1.0;
    vec4 dis = dt * dotDepth + 1.0 - tex * textureDepth;
    
    dis.x = dis.x - 1.0 + textureDepth*dotDepth;
    dis.y = dis.y - 1.0 + textureDepth*dotDepth;
    dis.x *= strength;
    dis.y *= strength;
    vec2 res_uv = texCoord ;
    res_uv.x = res_uv.x + dis.x - 0.0;
    res_uv.y = res_uv.y + dis.y;
    
    return res_uv;
}

float ease1(float t)
{
    return t == 0.0 || t == 1.0
    ? t
    : t < 0.5
    ? +0.5 * pow(2.0, (20.0 * t) - 10.0)
    : -0.5 * pow(2.0, 10.0 - (t * 20.0)) + 1.0;
}

float ease2(float t)
{
    return t == 1.0 ? t : 1.0 - pow(2.0, -10.0 * t);
}

void main()
{
    vec2 p = v_TextureCoordinates;
    vec4 color1 = texture2D(u_from, p);
    vec4 color2 = texture2D(u_to, p);
    vec2 disp = displace(color1, p, 0.33, 0.7, 1.0-ease1(u_progress));
    vec2 disp2 = displace(color2, p, 0.33, 0.5, ease2(u_progress));
    vec4 dColor1 = texture2D(u_to, disp);
    vec4 dColor2 = texture2D(u_from, disp2);
    float val = ease1(u_progress);
    vec3 gray = vec3(dot(min(dColor2, dColor1).rgb, vec3(0.299, 0.587, 0.114)));
    dColor2 = vec4(gray, 1.0);
    dColor2 *= 2.0;
    color1 = mix(color1, dColor2, smoothstep(0.0, 0.5, u_progress));
    color2 = mix(color2, dColor1, smoothstep(1.0, 0.5, u_progress));
    
    gl_FragColor = mix(color1, color2, val);
}
