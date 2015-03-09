#version 410 core

in block
{
    vec2 Texcoord;
} In;

uniform sampler2D Texture;
uniform float Gamma;

layout(location = 0, index = 0) out vec4 Color;

void main(void)
{
    vec3 color = texture(Texture, In.Texcoord).rgb;
    color.x = pow(color.x, (1/Gamma));
    color.y = pow(color.y, (1/Gamma));
    color.z = pow(color.z, (1/Gamma));
    Color = vec4(color, 1.0);
}
