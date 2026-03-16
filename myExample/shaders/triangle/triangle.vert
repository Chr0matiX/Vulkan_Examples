#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inPosApex;
layout (location = 4) in float inHeightRatio;

layout (binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
} ubo;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outNormal;
// 法线贴图相关
layout (location = 2) out vec3 outTangent;
layout (location = 3) out vec2 outUV;

out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main() 
{
	mat3 modelMatrix = mat3(ubo.modelMatrix);

	outColor = inColor;
	outNormal = modelMatrix * inNormal;
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);

	// 法线贴图部分
	// 此处不能直接单位化，存在0/0风险
	outTangent = cross(inNormal, vec3(0.0, 0.0, 1.0));
	// 点乘判空优化效率
	if (dot(outTangent, outTangent) < 0.001)
		outTangent = vec3(1.0, 0.0, 0.0);

	outTangent = normalize(outTangent);

	outTangent = modelMatrix * outTangent;
	
	vec3 vecPt2Apex = inPos - inPosApex;
	if (dot(vecPt2Apex, vecPt2Apex) < 0.001)
		vecPt2Apex = vec3(0.0, 0.0, 0.0);
	else
		vecPt2Apex = normalize(vecPt2Apex);

	vecPt2Apex = vecPt2Apex * 0.5 * inHeightRatio + vec3(0.5, 0.5, 0);
	outUV = vecPt2Apex.xy;
}
