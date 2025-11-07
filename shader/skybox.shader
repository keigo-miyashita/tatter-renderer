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
layout(location = 0) out vec4 ndc;

void main()
{
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 1.0, 1.0);

    ndc = vec4(gl_Position.xy, 1.0f, 0.0f);
}
#endif

#ifdef GL_FRAGMENT_SHADER
layout(location = 0) in vec4 ndc;
layout(location = 0) out vec4 outColor;

vec3 CorrectDirectionForEnvMap(vec3 dir) {
    return vec3(-dir.z, dir.y, dir.x);
}

void main()
{
    // ndc -> clip (w == 1)
    // ndc equals clip in homogeneous coordinates
    vec4 clip = vec4(ndc.xyz, 1.0);

    // clip -> view
    vec4 view = camera.invProj * clip;
    view /= view.w; // device for converting w = 1

    // view -> world
    vec4 world = camera.invView * view;

    vec4 dir = normalize(world);

    vec3 correctedDir = CorrectDirectionForEnvMap(dir.rgb);
    vec4 color = texture(cubeEnvMap, correctedDir);

    outColor = vec4(color.rgb, 1.0f);
}
#endif