#version 450
layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0, rgba16f) writeonly uniform image2D brdfLUT;

const float PI = 3.14159265359;

// Radical inverse for Hammersley sequence
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N) {
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

// GGX importance sampling
// N == (0, 0, 1) : process in tangent space
// N != (0, 0, 1) : process in world space
vec3 ImportanceSampleGGX(vec2 xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * xi.y;
    float cosTheta = sqrt((1.0 - xi.x)/(1.0 + (a * a - 1.0) * xi.x));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 h;
    h.x = cos(phi) * sinTheta;
    h.y = sin(phi) * sinTheta;
    h.z = cosTheta;

    // tangent space -> world space
    // If N is (0, 0, 1), T = (1, 0, 0u), B = (0, 1, 0))
    // so TBN become identity matrix and return tangent space vector directly
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0,0.0,1.0) : vec3(1.0,0.0,0.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);

    return normalize(T * h.x + B * h.y + N * h.z);
}

// Geometry (Smith-Schlick-GGX) model
float GeometrySmith(float NdotV, float NdotL, float roughness) {
    float a = roughness;
    float k = (a) * (a) / 8.0; // NOTE : different from direct lighting
    float Gv = NdotV / (NdotV * (1.0 - k) + k);
    float Gl = NdotL / (NdotL * (1.0 - k) + k);
    return Gv * Gl;
}

// Fresnel Schlick approximation
vec3 FresnelSchlick(float cosTheta) {
    return vec3(pow(1.0 - cosTheta, 5.0));
}

void main() {
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    const int size = 512;
    if(id.x >= size || id.y >= size) return;

    float NdotV = float(id.x + 0.5) / float(size); // x: NdotV
    float roughness = float(id.y + 0.5) / float(size); // y: roughness

    vec3 v; // tangent space
    v.x = sqrt(1.0 - NdotV * NdotV); // sin(theta), as a result v is normalized
    v.y = 0.0;
    v.z = NdotV;

    const int SAMPLE_COUNT = 1024;
    float A = 0.0;
    float B = 0.0;

    for(int i = 0; i < SAMPLE_COUNT; i++) {
        vec2 xi = Hammersley(i, SAMPLE_COUNT);
        vec3 h = ImportanceSampleGGX(xi, vec3(0.0, 0.0, 1.0), roughness);
        vec3 l = normalize(2.0 * dot(v,h) * h - v);

        float NdotL = max(l.z, 0.0); // tangent space
        float NdotH = max(h.z, 0.0); // tangent space
        float VdotH = max(dot(v,h), 0.0); // tangent space

        if(NdotL > 0.0) {
            float G = GeometrySmith(NdotV, NdotL, roughness);
            // float G_Vis = G * VdotH / (NdotH * NdotV);
            // This is equivalent  to factoring out F0 from FresnelSchlick in shading
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G * VdotH / (NdotH * NdotV);
            B += Fc * G * VdotH / (NdotH * NdotV);
        }
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);

    imageStore(brdfLUT, id, vec4(A, B, 0.0, 1.0));
}
