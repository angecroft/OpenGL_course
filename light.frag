#version 410 core

#define FRAG_COLOR	0

precision highp int;

uniform vec3 lightColor[4];

layout(location = FRAG_COLOR, index = 0) out vec4 FragColor;

in block
{
    vec3 Position;
} In;

void main()
{
    FragColor = vec4(lightColor[gl_PrimitiveID], 1);
}
