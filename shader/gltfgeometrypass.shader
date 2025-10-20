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

layout(set = 0, binding = 3) uniform sampler2D baseColorTexture;

layout(set = 0, binding = 4) uniform sampler2D metallicRoughnessTexture;

layout(set = 0, binding = 5) uniform sampler2D normalTexture;

layout(set = 0, binding = 6) uniform sampler2D occlusionTexture;

layout(set = 0, binding = 7) uniform sampler2D emissiveTexture;

layout(push_constant) uniform Factors
{
	vec4 baseColor;
	vec3 emissive;
	float padding;
	float metallic;
	float roughness;
} factors;



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
	vec3 normal = normalize(object.ITModel * vNormal).rgb;
	vec3 tangent = normalize(object.model * vTangent).rgb;
	tangent = normalize(tangent - dot(tangent, normal) * normal);
	vec3 bitangent = normalize(cross(normal, tangent)) * vTangent.w;
	mat3 TBN = mat3(tangent, bitangent, normal);

	vec3 normalMap = texture(normalTexture, vUV).xyz * 2.0 - 1.0;
	fNormal = vec4(normalize(TBN * normalMap), 0.0);  

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
	vec3 outputColor = texture(baseColorTexture, fUV).rgb * factors.baseColor.rgb;
	float metallic = texture(metallicRoughnessTexture, fUV).g * factors.metallic;
    outBaseColorMetallness = vec4(outputColor.rgb, metallic);

	vec3 normal = fNormal.rgb;
	float roughness = texture(metallicRoughnessTexture, fUV).b * factors.roughness;
    outNormalRoughness = vec4((normal.rgb + 1.0f) * 0.5f, roughness);

    // �ǉ����i��: roughness, metallic�j
	vec3 emissive = texture(emissiveTexture, fUV).rgb * factors.emissive;
	float ao = texture(occlusionTexture, fUV).r;
    outEmissiveAO = vec4(emissive.rgb, ao);
}
#endif