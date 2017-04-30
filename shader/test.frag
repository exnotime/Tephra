#version 430 core
layout(location = 0) in vec3 PosW;
layout(location = 1) in vec3 NormalW;
layout(location = 2) in vec3 TangentW;
layout(location = 3) in vec2 TexCoordOut;
layout(location = 4) in vec3 BiNormOut;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform WVP{
    mat4 wvp;
    vec4 CamPos;
    vec4 LightDir;
};
#include "lighting.glsl"

layout(set = 3, binding = 0) uniform sampler2D g_Material[4];

vec3 CalcBumpedNormal(vec3 Bump, vec3 Normal, vec3 Tangent, vec3 BiNorm){
    vec3 normal = normalize(Normal);
    vec3 tangent = normalize(Tangent);
    vec3 binorm = normalize(BiNorm);

    mat3 TBN = mat3(tangent,binorm,normal);
    vec3 newNormal = TBN * Bump;
    return normalize(newNormal);
}

void main(){
    vec3 bump = texture(g_Material[1], TexCoordOut).xyz * 2.0 - 1.0;
    vec3 normal = CalcBumpedNormal(bump, NormalW, TangentW,BiNormOut);
    vec3 lightDir = normalize(LightDir.xyz);
    vec3 toCam = normalize(CamPos.xyz - PosW);
    vec3 texColor = pow(texture(g_Material[0], TexCoordOut).rgb, vec3(2.2));
    float r = texture(g_Material[2], TexCoordOut).r;
    float m = texture(g_Material[3], TexCoordOut).r;
    vec3 lightColor = CalcDirLight(-lightDir, texColor, normal, toCam, r, m);
    lightColor += CalcIBLLight( normal, toCam, texColor, r, m) * 0.5;

    lightColor = ditherRGB(lightColor, gl_FragCoord.xy);

    outColor = saturate(vec4(lightColor, 1));
}