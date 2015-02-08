#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp float;
precision highp int;

uniform mat4 MVP;
uniform float Time;

layout(location = POSITION) in vec3 Position;
layout(location = NORMAL) in vec3 Normal;
layout(location = TEXCOORD) in vec2 TexCoord;

out gl_PerVertex
{
	vec4 gl_Position;
};

out block
{
        vec2 TexCoord;
        vec3 Position;
        vec3 Normal;
        float Time;
} Out;

void main()
{	
    vec3 pos = Position;
    vec3 normal = Normal;

    gl_Position = MVP * vec4(pos, 1.0);

    Out.TexCoord = TexCoord;
    Out.Position = Position;
    Out.Normal = normal;
    Out.Time = Time;
}
