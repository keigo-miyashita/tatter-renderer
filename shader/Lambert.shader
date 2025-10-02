#version 450

layout(std140, set = 0, binding = 0) uniform Camera /*type name*/
{
	mat4 view;
	mat4 proj;
} camera;

layout(std140, set = 0, binding = 1) uniform Object /*type name*/
{
	// add model uniforms here
	mat4 model;
	mat4 ITModel;
} object;

layout(std140, set = 0, binding = 2) uniform Light /*type name*/
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
	fWorldPosition = vPosition;
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

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 N = normalize(fNormal.xyz);
	vec3 L = normalize(light.lightPos.xyz);

	float diff = max(dot(N, L), 0.0);

	vec3 color = color.baseColor.rgb * light.lightColor.rgb * diff;

	outColor = vec4(color, 1.0);
}
#endif