#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inNormal;
// 法线贴图相关
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inUV;

// 绑定采样器
layout (binding = 1) uniform sampler2D normalMap;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 lightDir = normalize(vec3(-1.0, 0.5, 1.0));

	// 计算 TBN
	vec3 N = normalize(inNormal);
    vec3 T = normalize(inTangent);
    vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);

	vec3 tangentNormal = texture(normalMap, inUV).rgb * 2.0 - 1.0;
	vec3 n = normalize(TBN * tangentNormal);

	//vec3 n = normalize(inNormal);

	// 颜色比例
	//float diff = max(dot(n, lightDir), 0.4);
	//float diff = dot(n, lightDir) * 0.3 + 0.7;
	float diff = dot(n, lightDir) * 0.35 + 0.65;

	outFragColor = vec4(inColor * diff, 1.0);
}