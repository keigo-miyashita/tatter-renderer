#version 450

#define PI 3.14159265359

layout(std140, set = 0, binding = 0) uniform CameraDetail
{
	mat4 view;
	mat4 proj;
    mat4 invView;
    mat4 invProj;
    vec4 position;
    float nearZ;
    float farZ;
} camera;

layout(set = 0, binding = 1) uniform samplerCube cubeEnvMap;  // HDR input

#ifdef GL_VERTEX_SHADER
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTangent;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec4 ndc;

void main()
{
    // フルスクリーントライアングル（Y反転対応）
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0), // 左下 NDC
        vec2( 3.0, -1.0), // 右下 NDC
        vec2(-1.0,  3.0)  // 左上 NDC
    );

    gl_Position = vec4(positions[gl_VertexIndex], 1.0, 1.0);

    // vec2 uv = (gl_Position.xy * 0.5) + 0.5; // [-1,+1] → [0,1]
    ndc = vec4(gl_Position.xy, 1.0f, 0.0f);

    // // UV生成（Y反転補正）
    // fragUV = (gl_Position.xy * 0.5) + 0.5; // [-1,+1] → [0,1]
    // fragUV.y = 1.0 - fragUV.y;             // Y反転補正
}
#endif

#ifdef GL_FRAGMENT_SHADER
layout(location = 0) in vec4 ndc;
layout(location = 0) out vec4 outColor;

void main()
{
    // // 1. NDC 空間に変換（x, y, z は -1 〜 1 の範囲）
    // vec3 ndc;
    // ndc.xy = fragUV * 2.0 - 1.0;       // [0,1] → [-1,1]
    // ndc.z  = 1.0;    // [0,1] → [-1,1] (OpenGL の場合)

    // 2. クリップ空間座標（同次座標 w = 1）に変換
    vec4 clip = vec4(ndc.xyz, 1.0);

    // 3. クリップ空間 → ビュー空間（投影行列の逆行列を掛ける）
    vec4 view = camera.invProj * clip;

    // 透視除算の逆（w で割る）
    view /= view.w;

    // 4. ビュー空間 → ワールド空間（ビュー行列の逆行列を掛ける）
    vec4 world = camera.invView * view;

    vec4 dir = normalize(world);
    // vec4 color = texture(cubeEnvMap, dir.xyz);

    vec3 correctedDir = vec3(-dir.z, dir.y, dir.x);
    vec4 color = texture(cubeEnvMap, correctedDir);
    // vec4 color = texture(cubeEnvMap, normalize(vec3(1.0, dir.yz)));

    outColor = vec4(color.rgb, 1.0f);
}
#endif