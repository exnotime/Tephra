#version 430
layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D InputTex;

layout(binding = 1) uniform ToneMapUniformData {
	float Bright;
	float Exposure;
};

vec3 Uncharted2Tonemap(vec3 x)
{
    float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main(){
	outColor = texture(InputTex, TexCoord);
	// if(TexCoord.x > 0.5)
	// 	outColor.rgb = Uncharted2Tonemap(outColor.rgb);
	// else
	float Y = dot(vec4(0.30, 0.59, 0.11, 0.0), outColor);
    float YD = Exposure * (Exposure / Bright + 1.0) / (Exposure + 1.0);
	outColor *= YD;
}