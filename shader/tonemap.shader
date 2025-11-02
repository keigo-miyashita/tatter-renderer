#version 450

#define PI 3.14159265359

layout(set = 0, binding = 0) uniform sampler2D gRenderingResult;

#ifdef GL_VERTEX_SHADER
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

vec3 Reinhard(vec3 hdr) {
    return hdr / (hdr + vec3(1.0));
}

void main()
{
    vec3 hdrColor = texture(gRenderingResult, fragUV).rgb;
    vec3 toonMapedColor = Reinhard(hdrColor);

    outColor = vec4(toonMapedColor, 1.0);
}
#endif