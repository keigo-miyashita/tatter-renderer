#version 450

layout(std140, set = 0, binding = 0) uniform Camera
{
	mat4 view;
	mat4 proj;
} camera;

layout(std140, set = 0, binding = 1) uniform Object
{
	// add model uniforms here
	mat4 model;
	mat4 ITModel;
} object;

layout(std140, set = 0, binding = 2) uniform Light
{
	vec4 lightPos;
	vec4 lightColor;
} light;

layout(std140, set = 0, binding = 3) uniform Color
{
	vec4 baseColor;
} color;

// G-Buffer サンプラー
layout(set = 0, binding = 4) uniform sampler2D gBaseColorMetallness;
layout(set = 0, binding = 5) uniform sampler2D gNormalRoughness;
layout(set = 0, binding = 6) uniform sampler2D gEmissiveAO;

#ifdef GL_VERTEX_SHADER
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTangent;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec2 fragUV;

void main()
{
    // フルスクリーントライアングル（Y反転対応）
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0), // 左下 NDC
        vec2( 3.0, -1.0), // 右下 NDC
        vec2(-1.0,  3.0)  // 左上 NDC
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);

    // UV生成（Y反転補正）
    fragUV = (gl_Position.xy * 0.5) + 0.5; // [-1,+1] → [0,1]
    fragUV.y = 1.0 - fragUV.y;             // Y反転補正
}
#endif

#ifdef GL_FRAGMENT_SHADER
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main()
{
    vec3 baseColor  = texture(gBaseColorMetallness, fragUV).rgb;
    vec3 normal  = texture(gNormalRoughness, fragUV).rgb * 2.0f - 1.0f;
    float mr      = texture(gBaseColorMetallness, fragUV).a;
    float roughness = texture(gNormalRoughness, fragUV).a;
    float ao     = texture(gEmissiveAO, fragUV).a;

    // float metal  = mr.r;
    // float rough  = mr.g;

    // vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.5));
    // vec3 diffuse  = max(dot(normal, -lightDir), 0.0) * albedo;

    // vec3 ambient  = 0.1 * albedo * ao;

    vec3 N = normalize(normal.xyz);
	vec3 L = normalize(light.lightPos.xyz);

	float diff = max(dot(N, L), 0.0);

	vec3 outputColor = baseColor.rgb * light.lightColor.rgb * diff;
    outColor = vec4(outputColor, 1.0);
    // outColor = vec4(normal, 1.0);
    // outColor = vec4(vec3(1.0f, 0.0f, 0.0f), 1.0);
}
#endif