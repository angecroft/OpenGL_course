#version 410 core
in block
{
    vec2 Texcoord;
} In;

uniform sampler2D Texture;
uniform int isDepth;

layout(location = 0, index = 0) out vec4  Color;

void main(void)
{
    vec3 color;
    if(isDepth == 1)
    {
        vec3 depth = vec3(texture(Texture, In.Texcoord).r);
        color = depth.rgb;
    }
    else
    {
        color = texture(Texture, In.Texcoord).rgb;
    }
    Color = vec4(color, 1.0);
}
