#version 450
#ifdef GL_COMPUTE_SHADER
layout(local_size_x = 8, local_size_y = 8) in;

layout(binding = 0, rgba8) readonly uniform image2D rawHDR;
layout(binding = 1, rgba16f) uniform image2D decodedHDR;

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(rawHDR);

    if (coord.x >= size.x || coord.y >= size.y)
        return;

    vec4 rgbe = imageLoad(rawHDR, coord);

    float exponent = rgbe.a * 255.0;

    if (exponent > 0.0f) {
        float scale = exp2(exponent - 128.0);
        // vec3 color = rgbe.rgb * 255.0 * scale / 255.0;
        vec3 color = rgbe.rgb * scale;
        imageStore(decodedHDR, coord, vec4(color, 1.0));
        // imageStore(decodedHDR, coord, vec4(1.0,1.0,1.0, 1.0));
    } else {
        imageStore(decodedHDR, coord, vec4(0.0, 0.0, 0.0, 1.0));
    }
}
#endif