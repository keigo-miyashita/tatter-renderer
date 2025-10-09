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


#ifdef GL_VERTEX_SHADER
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTangent;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec4 fWorldPosition;
layout(location = 1) out vec4 fNormal;
layout(location = 2) out vec4 fTangent;
layout(location = 3) out vec2 fUV;

void main()
{
	fWorldPosition = object.model * vPosition;
	fNormal = normalize(object.ITModel * vNormal);
	fTangent = normalize(object.ITModel * vTangent);
	fUV = vUV;

	gl_Position = camera.proj * camera.view * fWorldPosition;
}
#endif

#ifdef GL_FRAGMENT_SHADER
layout(location = 0) in vec4 fWorldPosition;
layout(location = 1) in vec4 fNormal;
layout(location = 2) in vec4 fTangent;
layout(location = 3) in vec2 fUV;

layout(location = 0) out vec4 outBaseColorMetallness;   // Base Color + Metalness
layout(location = 1) out vec4 outNormalRoughness;   // gbuffer1
layout(location = 2) out vec4 outEmissiveAO;    // gbuffer2

void main()
{
	// アルベドはテクスチャ参照
	vec3 N = normalize(fNormal.xyz);
	vec3 L = normalize(light.lightPos.xyz);

	float diff = max(dot(N, L), 0.0);

	vec3 outputColor = color.baseColor.rgb * light.lightColor.rgb * diff;

    outBaseColorMetallness = vec4(outputColor, 1.0f);
    // outBaseColorMetallness = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    // outBaseColorMetallness = vec4(1.0f, 0.0f, 0.0f, 1.0f);

    // 法線を[0,1]に変換して格納
    outNormalRoughness = vec4(normalize(fNormal.rgb) * 0.5 + 0.5, 1.0);
    // outNormalRoughness = vec4(1.0f, 0.0f, 0.0f, 1.0f);

    // 追加情報（例: roughness, metallic）
    outEmissiveAO = vec4(0.0, 0.0, 0.0, 1.0);
    // outEmissiveAO = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
#endif