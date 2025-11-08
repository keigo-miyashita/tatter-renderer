#version 450
#ifdef GL_COMPUTE_SHADER

#define PI 3.14159265359

layout(local_size_x = 8, local_size_y = 8) in;

layout(binding = 0) uniform sampler2D decodedHDR;
layout(binding = 1, rgba16f) writeonly uniform imageCube cubeEnvMap;  // HDR input
layout(std140, binding = 2) uniform Sizes
{
    ivec2 hdrSize;
    ivec2 pad1;
    ivec2 cubeSize;
    ivec2 pad2;
} sizes;

// cubemap face indexing
layout(push_constant) uniform Push {
    int face;   // 0..5
} push;

vec3 GetDirection(int face, vec2 uv)
{
    // Coordinates of each face (0, 0, 0)
    // so each face goes from -1 to +1 in both u and v
    uv = uv * 2.0 - 1.0; // [0,1] ¨ [-1,1]
    vec3 dir = vec3(0.0);

    // +X (when +x is max)
         if (face == 0)     dir = normalize(vec3( 1.0,  -uv.y, -uv.x));
    // -X (when -x is max)
    else if (face == 1)     dir = normalize(vec3(-1.0,  -uv.y,  uv.x));
    // +Y (when +y is max)
    else if (face == 2)     dir = normalize(vec3( uv.x,   1.0, uv.y));
    // -Y (when -y is max)
    else if (face == 3)     dir = normalize(vec3( uv.x,  -1.0,  -uv.y));
    // +Z (when +z is max)
    else if (face == 4)     dir = normalize(vec3(uv.x,  -uv.y,  1.0));
    // -Z (when -z is max)
    else if (face == 5)     dir = normalize(vec3(-uv.x, -uv.y,  -1.0));

    return dir;
}

vec2 DirectionToEquirectUV(vec3 dir)
{
    float theta = atan(dir.z, dir.x); // [-PI, PI]]
    float phi   = asin(clamp(dir.y, -1.0, 1.0)); // [-PI/2, PI/2]

    vec2 uv;
    uv.x = (theta / (2.0 * PI)) + 0.5;
    uv.y = (phi / PI) + 0.5;
    uv.y = 1.0 - uv.y; // Flip V
    return uv;
}

void main()
{
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);

    if (id.x >= sizes.cubeSize.x || id.y >= sizes.cubeSize.y)
        return;

    vec2 uv = (vec2(id) + 0.5) / (sizes.cubeSize);
    vec3 dir = GetDirection(push.face, uv);
    vec2 envUV = DirectionToEquirectUV(dir);

    vec3 color = texture(decodedHDR, envUV).rgb;
    imageStore(cubeEnvMap, ivec3(id, push.face), vec4(color, 1.0));
    // imageStore(cubeEnvMap, ivec3(id, push.face), vec4(id.x, id.y, 1.0, 1.0));
    // imageStore(cubeEnvMap, ivec3(id, push.face), vec4(sizes.hdrSize.x, sizes.hdrSize.y, 1.0, 1.0));
    // imageStore(cubeEnvMap, ivec3(0, 0, push.face), vec4(1.0f, 1.0f, 1.0f, 1.0f));
    // imageStore(cubeEnvMap, ivec3(id, push.face), vec4(1.0,1.0,1.0, 1.0));
}
#endif