#version 450

layout(std140, set = 0, binding = 0) uniform CameraDetail
{
	mat4 view;
	mat4 proj;
    mat4 invView;
    mat4 invProj;
    vec4 position;
} camera;

layout(std140, set = 0, binding = 1) uniform Object
{
	mat4 model;
	mat4 ITModel;
} object;

layout(std140, set = 0, binding = 2) uniform Light
{
	vec4 lightPos;
	vec4 lightColor;
} light;

layout(set = 0, binding = 3) uniform sampler2D gBaseColorMetallness;
layout(set = 0, binding = 4) uniform sampler2D gNormalRoughness;
layout(set = 0, binding = 5) uniform sampler2D gEmissiveAO;
layout(set = 0, binding = 6) uniform sampler2D gDepth;

#ifdef GL_VERTEX_SHADER
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTangent;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec2 fragUV;

void main()
{
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0), 
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)  
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);

    fragUV = (gl_Position.xy * 0.5) + 0.5;
    fragUV.y = 1.0 - fragUV.y;   
}
#endif

#ifdef GL_FRAGMENT_SHADER
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

vec3 ReconstructWorldPosition(vec2 uv, float depth)
{
    vec3 ndc;
    ndc.xy = uv * 2.0 - 1.0;
    ndc.z  = depth * 2.0 - 1.0;

    vec4 clip = vec4(ndc, 1.0);

    vec4 view = cameraInv.invProj * clip;

    view /= view.w;

    vec4 world = cameraInv.invView * view;

    return world.xyz;
}


vec3 FresnelSchlick(float cosTheta, vec3 F0, vec3 F90) {
    float powTerm = pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    return mix(F0, F90, powTerm);
}

float DistributionGGX(float NdotH, float alphaRoughness) {
    float a = alphaRoughness;
    float a2 = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float GeometrySmith(float NdotV, float NdotL, float alphaRoughness) {
    float a = alphaRoughness;
    float k = (a + 1.0) * (a + 1.0) / 8.0;
    float Gv = NdotV / (NdotV * (1.0 - k) + k);
    float Gl = NdotL / (NdotL * (1.0 - k) + k);
    return Gv * Gl;
}

vec3 DiffuseLambert(vec3 diffuseColor) {
    return diffuseColor / PI;
}

void main()
{
    vec3 baseColor  = texture(gBaseColorMetallness, fragUV).rgb;
    vec3 normal  = texture(gNormalRoughness, fragUV).rgb * 2.0f - 1.0f;
    float metalic      = texture(gBaseColorMetallness, fragUV).a;
    float roughness = texture(gNormalRoughness, fragUV).a;
    float ao     = texture(gEmissiveAO, fragUV).a;
    float emissive = texture(gEmissiveAO, fragUV).rgb;
    float depth = texture(gDepth, fragUV).r;

    vec3 worldPos = ReconstructWorldPosition(fragUV, depth);

    vec3 v = normalize(camera.position - worldPos);
    vec3 l = normalize(light.lightPos.xyz - worldPos);
    vec3 h = normalize(v + l);
    vec3 reflection = reflect(-v, normal);

    float NdotL = clamp(dot(normal, l), 0.001, 1.0);
    float NdotV = clamp(dot(abs(normal, v)), 0.001, 1.0);
    float NdotH = clamp(dot(normal, h), 0.001, 1.0);
    float VdotH = clamp(dot(v, h), 0.001, 1.0);

    vec3 dielectoricF0 = vec3(0.04);
    vec3 specularColor = mix(dielectoricF0, baseColor, metalic);
    vec3 diffuseColor = baseColor * (1.0 - metalic);

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

    // NOTE : Add AO

    outColor = vec4(direct + emissive, 1.0);
}
#endif