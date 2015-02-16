#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp int;

uniform sampler2D Diffuse;
uniform sampler2D Diffuse2;

uniform int specularPower;

//layout(location = FRAG_COLOR, index = 0) out vec4 FragColor;
// Write in GL_COLOR_ATTACHMENT0
layout(location = 0 ) out vec4 Color;
// Write in GL_COLOR_ATTACHMENT1
layout(location = 1) out vec4 Normal;

in block
{
    vec2 TexCoord;
    vec3 Position;
    vec3 Normal;
    float Time;
} In;

void main()
{
    vec3 diffuseColor = texture(Diffuse, In.TexCoord).rgb;
    vec3 spec = texture(Diffuse2, In.TexCoord).rgb;

    Normal.rgb = In.Normal;
    Normal.a = specularPower;

    Color.rgb = diffuseColor;
    Color.a = spec.r;
}
