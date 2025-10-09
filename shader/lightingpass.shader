#version 450

// G-Buffer サンプラー
layout(set = 0, binding = 0) uniform sampler2D gBaseColorMetallness;
layout(set = 0, binding = 1) uniform sampler2D gNormalRoughness;
layout(set = 0, binding = 2) uniform sampler2D gEmissiveAO;

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
    vec3 normal  = texture(gNormalRoughness, fragUV).rgb;
    float mr      = texture(gBaseColorMetallness, fragUV).a;
    float roughness = texture(gNormalRoughness, fragUV).a;
    float ao     = texture(gEmissiveAO, fragUV).a;

    // float metal  = mr.r;
    // float rough  = mr.g;

    // vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.5));
    // vec3 diffuse  = max(dot(normal, -lightDir), 0.0) * albedo;

    // vec3 ambient  = 0.1 * albedo * ao;

    // outColor = vec4(diffuse + ambient, 1.0);
    // if (baseColor.r >= 1.0f) {
    //     outColor = vec4(0.0f, 1.0f, 0.0f, 1.0);
    // } else {
    //     outColor = vec4(baseColor, 1.0);
    // }
    outColor = vec4(baseColor, 1.0);
    // outColor = vec4(normal, 1.0);
    // outColor = vec4(vec3(1.0f, 0.0f, 0.0f), 1.0);
}
#endif