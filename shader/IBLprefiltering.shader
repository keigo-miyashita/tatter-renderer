#version 450
layout(local_size_x = 8, local_size_y = 8) in;

layout(binding = 0) uniform sampler2D rawHDR;
layout(binding = 1) uniform sampler2D decodedHDR;
layout(binding = 2) uniform samplerCube uEnvMap;  // HDR input
layout(binding = 3d) uniform writeonly imageCube uDiffuseMap;

const float PI = 3.14159265359;

// cubemap face indexing
layout(push_constant) uniform Push {
    int face;   // 0..5
    int size;   // face resolution
} push;

// vec3 SampleHemisphere(vec3 normal, vec2 Xi) {
//     float phi = 2.0 * PI * Xi.x;
//     float cosTheta = sqrt(1.0 - Xi.y);
//     float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

//     vec3 H;
//     H.x = cos(phi) * sinTheta;
//     H.y = sin(phi) * sinTheta;
//     H.z = cosTheta;

//     // tangent -> world not needed if normal is +Z in cube face
//     return H;
// }

// void main() {
//     ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
//     int size = push.size;
//     if (pixel.x >= size || pixel.y >= size) return;

//     vec2 uv = (vec2(pixel) + 0.5) / float(size);
//     vec3 normal = normalize(vec3(uv * 2.0 - 1.0, 1.0)); // +Z face example

//     vec3 irradiance = vec3(0.0);
//     const int SAMPLE_COUNT = 1024;
//     for(int i=0;i<SAMPLE_COUNT;i++){
//         vec2 Xi = vec2(
//             fract(sin(float(i) * 12.9898) * 43758.5453),
//             fract(sin(float(i) * 78.233) * 43758.5453)
//         );
//         vec3 sampleDir = SampleHemisphere(normal, Xi);
//         irradiance += texture(uEnvMap, sampleDir).rgb * dot(sampleDir, normal);
//     }
//     irradiance = irradiance * (PI / float(SAMPLE_COUNT));

//     imageStore(uDiffuseMap, ivec3(pixel, push.face), vec4(irradiance,1.0));
// }

void main()
{
    ivec2 size = imageSize(hdrOutput);
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);

    if (id.x >= size.x || id.y >= size.y)
        return;

    vec4 rgbe = texelFetch(rgbeInput, id, 0);

    if (exponent > 0.0f) {
        float scale = exp2(exponent - 128.0);
        vec3 linearColor = rgbe.rgb * scale;
        imageStore(hdrOutput, id, vec4(linearColor, 1.0));
    } else {
        imageStore(hdrOutput, id, vec4(0.0, 0.0, 0.0, 1.0));
    }
}
