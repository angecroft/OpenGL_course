#version 410 core
in block
{
    vec2 Texcoord;
} In;
uniform sampler2D ColorBuffer;
uniform sampler2D NormalBuffer;
uniform sampler2D DepthBuffer;

layout(location = 0) out vec4 Color;

void main(void)
{
    Color = vec4(1.0, 0.0, 1.0, 1.0);
}
