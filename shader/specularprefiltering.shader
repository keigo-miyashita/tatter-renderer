#version 450
#ifdef GL_COMPUTE_SHADER

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform samplerCube envMap; 
layout(set = 0, binding = 1, rgba16f) writeonly uniform imageCube prefilterMapMip0;
layout(set = 0, binding = 2, rgba16f) writeonly uniform imageCube prefilterMapMip1;
layout(set = 0, binding = 3, rgba16f) writeonly uniform imageCube prefilterMapMip2;
layout(set = 0, binding = 4, rgba16f) writeonly uniform imageCube prefilterMapMip3;
layout(set = 0, binding = 5, rgba16f) writeonly uniform imageCube prefilterMapMip4;
layout(set = 0, binding = 6, rgba16f) writeonly uniform imageCube prefilterMapMip5;
layout(set = 0, binding = 7, rgba16f) writeonly uniform imageCube prefilterMapMip6;
layout(set = 0, binding = 8, rgba16f) writeonly uniform imageCube prefilterMapMip7;
layout(set = 0, binding = 9, rgba16f) writeonly uniform imageCube prefilterMapMip8;
layout(set = 0, binding = 10, rgba16f) writeonly uniform imageCube prefilterMapMip9;

layout(push_constant) uniform Push
{
    int face;
    int size;
    int sampleCount;
    float roughness;
} push;

const float PI = 3.14159265359;

// Hammersley sequence
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // /2^32
}

vec2 Hammersley(uint i, uint N){
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

// GGX importance sampling
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.y;
    float cosTheta = sqrt((1.0 - Xi.x) / (1.0 + (a*a - 1.0) * Xi.x));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // tangent space -> world space
    vec3 up = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);

    vec3 sampleVec = normalize(T * H.x + B * H.y + N * H.z);
    return sampleVec;
}

vec3 GetDirection(int face, vec2 uv)
{
    // Coordinates of each face (0, 0, 0)
    // so each face goes from -1 to +1 in both u and v
    uv = uv * 2.0 - 1.0; // [0,1] → [-1,1]
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

void main() {
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    if(id.x >= push.size || id.y >= push.size) return;

    vec2 uv = (vec2(id) + 0.5) / float(push.size);
    uv = uv * 2.0 - 1.0;

    vec3 N;
    N = GetDirection(push.face, uv);
    vec3 R = N;
    vec3 V = R; // assume normal aligned view vector for prefilter

    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;
    for(uint i=0u;i<uint(push.sampleCount);i++){
        vec2 Xi = Hammersley(i,uint(push.sampleCount));
        vec3 H = ImportanceSampleGGX(Xi, N, push.roughness);
        vec3 L = normalize(2.0 * dot(V,H) * H - V);

        float NdotL = max(dot(N,L),0.0);
        if(NdotL > 0.0){
            vec3 sampleColor = texture(envMap,L).rgb;
            prefilteredColor += sampleColor * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;

    float roughnessGrad = 1.0f / 10;
    int mipLevel = int(push.roughness / roughnessGrad);
    switch(mipLevel){
        case 0:
            imageStore(prefilterMapMip0, ivec3(id, push.face), vec4(prefilteredColor, 1.0));
            break;
        case 1:
            imageStore(prefilterMapMip1, ivec3(id, push.face), vec4(prefilteredColor, 1.0));
            break;
        case 2:
            imageStore(prefilterMapMip2, ivec3(id, push.face), vec4(prefilteredColor, 1.0));
            break;
        case 3:
            imageStore(prefilterMapMip3, ivec3(id, push.face), vec4(prefilteredColor, 1.0));
            break;
        case 4:
            imageStore(prefilterMapMip4, ivec3(id, push.face), vec4(prefilteredColor, 1.0));
            break;
        case 5:
            imageStore(prefilterMapMip5, ivec3(id, push.face), vec4(prefilteredColor, 1.0));
            break;
        case 6:
            imageStore(prefilterMapMip6, ivec3(id, push.face), vec4(prefilteredColor, 1.0));
            break;
        case 7:
            imageStore(prefilterMapMip7, ivec3(id, push.face), vec4(prefilteredColor, 1.0));
            break;
        case 8:
            imageStore(prefilterMapMip8, ivec3(id, push.face), vec4(prefilteredColor, 1.0));
            break;
        case 9:
            imageStore(prefilterMapMip9, ivec3(id, push.face), vec4(prefilteredColor, 1.0));
            break;
    }
}
#endif
