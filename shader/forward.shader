#version 450

#define MAX_MIP_LEVEL 10
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

// G-Buffer サンプラー
layout(set = 0, binding = 8) uniform samplerCube irradianceMap;
layout(set = 0, binding = 9) uniform samplerCube prefilterMap;
layout(set = 0, binding = 10) uniform sampler2D brdfLUT;

layout(push_constant) uniform Factors
{
	vec4 baseColor;
	vec3 emissive;
	float padding;
	float metallic;
	float roughness;
    float padding1;
	float padding2;
	mat4 gltfModel;
	mat4 gltfITModel;
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
layout(location = 4) out vec4 glPosition; // debug

void main()
{
    mat4 modelMat = object.model * factors.gltfModel;
	mat3 invTransMat = transpose(inverse(mat3(object.model * factors.gltfModel)));
	fWorldPosition = modelMat * vPosition;
	vec3 normal = normalize(mat3(invTransMat) * vNormal.rgb).rgb;
	vec3 tangent = normalize(mat3(modelMat) * vTangent.rgb).rgb;
	tangent = normalize(tangent - dot(tangent, normal) * normal);
	vec3 bitangent = normalize(cross(normal, tangent)) * vTangent.w;
	mat3 TBN = mat3(tangent, bitangent, normal);

	vec3 normalMap = texture(normalTexture, vUV).xyz * 2.0 - 1.0;
	fNormal = vec4(normalize(TBN * normalMap), 0.0);  
	fNormal = vec4(normal, 0.0);  // NOTE : Unable normal map

	fUV = vUV;

	gl_Position = camera.proj * camera.view * fWorldPosition;
    glPosition = gl_Position; // debug
}
#endif

#ifdef GL_FRAGMENT_SHADER
layout(location = 0) in vec4 fWorldPosition;
layout(location = 1) in vec4 fNormal;
layout(location = 2) in vec4 fTangent;
layout(location = 3) in vec2 fUV;
layout(location = 4) in vec4 glPosition; // debug

layout(location = 0) out vec4 outColor;   // Base Color + Metalness

float LinearizeDepth(float depth, float nearZ, float farZ)
{
    return (nearZ * farZ) / (farZ - depth * (farZ - nearZ));
}

// Fresnel: Schlick approximation
// F0 + (F90 - F0) * (1 - cosTheta)^5
vec3 FresnelSchlick(float cosTheta, vec3 F0, vec3 F90) {
    float powTerm = pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    return F0 + (F90 - F0) * powTerm;
}

// D: GGX / Trowbridge-Reitz
float DistributionGGX(float NdotH, float roughness) {
    float a = roughness;
    float a2 = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// Geometry (Smith-Schlick-GGX) model
float GeometrySmith(float NdotV, float NdotL, float roughness) {
    float a = roughness;
    float k = (a + 1.0) * (a + 1.0) / 8.0;
    float Gv = NdotV / (NdotV * (1.0 - k) + k);
    float Gl = NdotL / (NdotL * (1.0 - k) + k);
    return Gv * Gl;
}

// Lambert diffuse
vec3 DiffuseLambert(vec3 diffuseColor) {
    return diffuseColor / PI;
}

vec3 CorrectDirectionForEnvMap(vec3 dir) {
    return vec3(-dir.z, dir.y, dir.x);
}

void main()
{
    vec3 baseColor      = texture(baseColorTexture, fUV).rgb * factors.baseColor.rgb;
    vec3 normal         = fNormal.rgb;
    float metallic      = texture(metallicRoughnessTexture, fUV).g * factors.metallic;
    float roughness     = texture(metallicRoughnessTexture, fUV).b * factors.roughness;
    vec3 emissive       = texture(emissiveTexture, fUV).rgb * factors.emissive;
	float ao            = texture(occlusionTexture, fUV).r;

    vec3 worldPos = fWorldPosition.rgb;

    vec3 v = normalize(camera.position.xyz - worldPos);
    vec3 l = normalize(light.lightPos.xyz - worldPos);
    vec3 h = normalize(v + l);
    vec3 r = reflect(-v, normal);

    float NdotL = clamp(dot(normal, l), 0.001, 1.0);
    float NdotV = clamp(abs(dot(normal, v)), 0.001, 1.0);
    float NdotH = clamp(dot(normal, h), 0.001, 1.0);
    float VdotH = clamp(dot(v, h), 0.001, 1.0);

    // if metallic = 0 -> 0.04, metallic = 1 -> baseColor
    vec3 specularColor = mix(vec3(0.04), baseColor, metallic);
    vec3 diffuseColor = baseColor * (1.0 - metallic);

    // Fresnel term
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
    // float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    vec3 F0 = specularColor;
    vec3 F90 = vec3(1.0f); // Fixed value based on glTF Sample viewer

    vec3  F = FresnelSchlick(VdotH, F0, F90);
    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);

    vec3 diffuseTerm = (1.0f - F) * DiffuseLambert(diffuseColor);
    vec3 specularColorTerm = (F * D * G) / (4.0 * NdotL * NdotV + 0.001);

    vec3 direct = NdotL * light.lightColor.rgb * (diffuseTerm + specularColorTerm);

    // vec3 diffuseIBL     = texture(irradianceMap, CorrectDirectionForEnvMap(normal)).rgb * baseColor * (1.0 - metallic);
    vec3 diffuseIBL     = texture(irradianceMap, CorrectDirectionForEnvMap(normal)).rgb * baseColor;
    vec3 prefiltered    = textureLod(prefilterMap, CorrectDirectionForEnvMap(r), roughness * MAX_MIP_LEVEL).rgb;
    vec2 brdf           = texture(brdfLUT, vec2(NdotV, roughness)).rg;
    vec3 specularIBL    = prefiltered * (F0 * brdf.x + brdf.y);
    // vec3 specularIBL    = prefiltered * (F * brdf.x + brdf.y);
    vec3 LIBL           = diffuseIBL + specularIBL;

    // NOTE : Add AO

    outColor = vec4(direct + emissive + LIBL, 1.0);
    // outColor = vec4(worldPos, 1.0);
    // outColor = vec4(normal * 0.5 + 0.5, 1.0f);
    // outColor = vec4(abs(dot(normal, v)),abs(dot(normal, v)),abs(dot(normal, v)), 1.0f);
    // outColor = vec4(v, 1.0f);
    // outColor = vec4(fNormal.rgb * 0.5 + 0.5, 1.0f);
}
#endif