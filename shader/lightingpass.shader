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

layout(std140, set = 0, binding = 1) uniform Light
{
	vec4 lightPos;
	vec4 lightColor;
} light;

// G-Buffer
layout(set = 0, binding = 2) uniform sampler2D gBaseColorMetallness;
layout(set = 0, binding = 3) uniform sampler2D gNormalRoughness;
layout(set = 0, binding = 4) uniform sampler2D gEmissiveAO;
layout(set = 0, binding = 5) uniform sampler2D gDepth;

// prefiltered environment maps
layout(set = 0, binding = 6) uniform samplerCube irradianceMap;
layout(set = 0, binding = 7) uniform samplerCube prefilterMap;
layout(set = 0, binding = 8) uniform sampler2D brdfLUT;

#ifdef GL_VERTEX_SHADER
layout(location = 0) out vec2 fragUV;

void main()
{
    // Full screen triangle (Y-flip compatible)
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0), // left-bottom NDC
        vec2( 3.0, -1.0), // right-bottom NDC
        vec2(-1.0,  3.0)  // left-top NDC
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);

    // generate UV
    fragUV = (gl_Position.xy * 0.5) + 0.5; // [-1,+1] → [0,1]
    fragUV.y = 1.0 - fragUV.y;             // Y-flip correction
}
#endif

#ifdef GL_FRAGMENT_SHADER
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

// NOTE : Check algorithm
float LinearizeDepth(float depth, float nearZ, float farZ)
{
    return (nearZ * farZ) / (farZ - depth * (farZ - nearZ));
}

vec3 ReconstructWorldPosition(vec2 uv, float depth)
{
    // 1. NDC 空間に変換（x, y, z は -1 〜 1 の範囲）
    vec3 ndc;
    ndc.xy = uv * 2.0 - 1.0;       // [0,1] → [-1,1]
    ndc.z  = depth * 2.0 - 1.0;    // [0,1] → [-1,1] (OpenGL の場合)

    // 2. クリップ空間座標（同次座標 w = 1）に変換
    vec4 clip = vec4(ndc, 1.0);

    // 3. クリップ空間 → ビュー空間（投影行列の逆行列を掛ける）
    vec4 view = camera.invProj * clip;

    // 透視除算の逆（w で割る）
    view /= view.w;

    // 4. ビュー空間 → ワールド空間（ビュー行列の逆行列を掛ける）
    vec4 world = camera.invView * view;

    return world.xyz;
}

// ---------------------------
// マイクロファセット関連関数（理論実装）
//   - D: GGX (Trowbridge-Reitz)
//   - G: Smith-Schlick-GGX（Karis / UE4）
//   - F: Schlick の近似（色ベース）
// ---------------------------

// Fresnel: Schlick近似（色ベース）
vec3 FresnelSchlick(float cosTheta, vec3 F0, vec3 F90) {
    // F0 + (F90 - F0) * (1 - cosTheta)^5
    float powTerm = pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    return mix(F0, F90, powTerm);
}

// D: GGX / Trowbridge-Reitz
float DistributionGGX(float NdotH, float alphaRoughness) {
    float a = alphaRoughness;
    float a2 = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// Geometry (Smith-Schlick-GGX) - UE4の式寄り
float GeometrySmith(float NdotV, float NdotL, float alphaRoughness) {
    // UE4 uses: k = (alpha + 1)^2 / 8, G1 = NdotX / (NdotX * (1 - k) + k)
    float a = alphaRoughness;
    float k = (a + 1.0) * (a + 1.0) / 8.0;
    float Gv = NdotV / (NdotV * (1.0 - k) + k);
    float Gl = NdotL / (NdotL * (1.0 - k) + k);
    return Gv * Gl;
}

// Lambertian diffuse (energy-conserving)
vec3 DiffuseLambert(vec3 diffuseColor) {
    return diffuseColor / PI;
}

void main()
{
    vec3 baseColor  = texture(gBaseColorMetallness, fragUV).rgb;
    // Correct range from [0,1] to [-1,+1] to [0,1]
    vec3 normal  = texture(gNormalRoughness, fragUV).rgb * 2.0f - 1.0f;
    float metalic      = texture(gBaseColorMetallness, fragUV).a;
    float roughness = texture(gNormalRoughness, fragUV).a;
    float ao     = texture(gEmissiveAO, fragUV).a;
    vec3 emissive = texture(gEmissiveAO, fragUV).rgb;
    float depth = LinearizeDepth(texture(gDepth, fragUV).r, camera.nearZ, camera.farZ);

    vec3 worldPos = ReconstructWorldPosition(fragUV, depth);

    vec3 v = normalize(camera.position.xyz - worldPos);
    vec3 l = normalize(light.lightPos.xyz - worldPos);
    vec3 h = normalize(v + l);
    vec3 reflection = reflect(-v, normal);

    float NdotL = clamp(dot(normal, l), 0.001, 1.0);
    float NdotV = clamp(abs(dot(normal, v)), 0.001, 1.0);
    float NdotH = clamp(dot(normal, h), 0.001, 1.0);
    float VdotH = clamp(dot(v, h), 0.001, 1.0);

    vec3 dielectoricF0 = vec3(0.04); // F0 for non-metallic
    vec3 specularColor = mix(dielectoricF0, baseColor, metalic);
    vec3 diffuseColor = baseColor * (1.0 - metalic);

    // フレネル項
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    vec3 F0 = specularColor;
    vec3 F90 = vec3(reflectance90);

    vec3 F = FresnelSchlick(VdotH, F0, F90);
    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);

    vec3 diffuseTerm = (1.0f - F) * DiffuseLambert(diffuseColor);
    vec3 specularColorTerm = (F * D * G) / (4.0 * NdotL * NdotV + 0.001);

    vec3 direct = NdotL * light.lightColor.rgb * (diffuseTerm + specularColorTerm);

    vec3 diffuseIBL = texture(irradianceMap, normal).rgb * baseColor * (1.0 - metalic);
    vec3 prefiltered = textureLod(prefilterMap, reflection, roughness * MAX_MIP_LEVEL).rgb;
    vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;
    vec3 specularIBL = prefiltered * (F0 * brdf.x + brdf.y);
    vec3 LIBL = diffuseIBL + specularIBL;

    // NOTE : Add AO

    outColor = vec4(direct + emissive + LIBL, 1.0);
    // outColor = vec4(worldPos, 1.0);
    // outColor = vec4(depth,depth,depth, 1.0);
    // float linearDepth = LinearizeDepth(depth, 0.1, 100.0); // near, farは適宜
    // outColor = vec4(vec3(linearDepth / 100.0), 1.0);
    // outColor = vec4(F, 1.0);
    // outColor = vec4(diffuseTerm, 1.0);
}
#endif