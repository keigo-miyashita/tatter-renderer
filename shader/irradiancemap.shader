#version 450
#ifdef GL_COMPUTE_SHADER
layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform samplerCube envMap;
layout(set = 0, binding = 1, rgba16f) writeonly uniform imageCube irradianceMap;

layout(push_constant) uniform Push
{
    int face;
    int size;
    int sampleCount;
} push;

const float PI = 3.14159265359;

// Hammersley sequence for quasi-random stratification
// return float in 0 - 1
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // /2^32
}

// return vec2 in [0,1]
vec2 Hammersley(uint i, uint N){
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

// cosine-weighted hemisphere sampling in tangent space
vec3 SampleHemisphereCosine(vec2 xi) {
    float r = sqrt(xi.x);
    float phi = 2.0 * PI * xi.y;
    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - xi.x));
    return vec3(x, y, z); // z is cos(theta)
}

// build tangent/bitangent for given normal
void BuildTangents(vec3 N, out vec3 T, out vec3 B) {
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    T = normalize(cross(up, N));
    B = normalize(cross(N, T)); // right handed
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
    if (id.x >= push.size || id.y >= push.size) return;

    vec2 uv = (vec2(id) + 0.5) / float(push.size); // [0,1]
    uv = uv * 2.0 - 1.0; // [-1,1]

    // compute normal direction for this face
    vec3 N, T, B;
    N = GetDirection(push.face, uv);
    // build tangent space
    BuildTangents(N, T, B);

    vec3 accum = vec3(0.0);
    uint Nsamples = uint(push.sampleCount);
    for (uint i = 0u; i < Nsamples; ++i) {
        vec2 xi = Hammersley(i, Nsamples);
        vec3 local = SampleHemisphereCosine(xi); // (x,y,z) where z = cos(theta)
        // world sample
        vec3 sampleDir = normalize(local.x * T + local.y * B + local.z * N);

        vec3 Li = texture(envMap, sampleDir).rgb;
        accum += Li;
    }

    // E(N) = Sum / N / (pdf) = Sum / N / (cos(theta) / PI) = (Sum * PI) / (N * cos(theta))
    vec3 irradiance = accum * (PI / float(Nsamples));

    imageStore(irradianceMap, ivec3(id, push.face), vec4(irradiance, 1.0));
}
#endif