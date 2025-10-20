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
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness*roughness;
    float phi = 2.0 * PI * Xi.y;
    float cosTheta = sqrt((1.0 - Xi.x)/(1.0 + (a*a - 1.0)*Xi.x));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    vec3 H;
    H.x = cos(phi)*sinTheta;
    H.y = sin(phi)*sinTheta;
    H.z = cosTheta;

    // tangent space -> world space
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0,0.0,1.0) : vec3(1.0,0.0,0.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);

    return normalize(T*H.x + B*H.y + N*H.z);
}

// geometry term (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float a = roughness;
    float k = (a*a) / 2.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// geometry smith
float GeometrySmith(float NdotV, float NdotL, float roughness) {
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

// Fresnel Schlick approximation
vec3 FresnelSchlick(float cosTheta) {
    return vec3(pow(1.0 - cosTheta, 5.0));
}

void main() {
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    const int size = 512;
    if(id.x >= size || id.y >= size) return;

    float NdotV = float(id.x + 0.5)/float(size); // x: NdotV
    float roughness = float(id.y + 0.5)/float(size); // y: roughness

    vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV); // sinθ
    V.y = 0.0;
    V.z = NdotV;

    const uint SAMPLE_COUNT = 1024u;
    float A = 0.0;
    float B = 0.0;

    for(uint i = 0u; i < SAMPLE_COUNT; i++) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, vec3(0.0,0.0,1.0), roughness);
        vec3 L = normalize(2.0*dot(V,H)*H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V,H), 0.0);

        if(NdotL > 0.0) {
            float G = GeometrySmith(NdotV, NdotL, roughness);
            float G_Vis = G * VdotH / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);

    imageStore(brdfLUT, id, vec4(A,B,0.0,1.0));
}
