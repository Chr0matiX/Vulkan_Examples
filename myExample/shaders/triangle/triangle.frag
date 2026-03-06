#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  vec3 lightDir = normalize(vec3(-1.0, 0.5, 1.0));
  vec3 n = normalize(inNormal);
  //float diff = max(dot(n, lightDir), 0.4);
  //float diff = dot(n, lightDir) * 0.3 + 0.7;
  float diff = dot(n, lightDir) * 0.35 + 0.65;

  outFragColor = vec4(inColor * diff, 1.0);
}