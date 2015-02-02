#version 410 core

#define POSITION	0

precision highp float;
precision highp int;

uniform mat4 MVP;

layout(location = POSITION) in vec3 Position;

out gl_PerVertex
{
        vec4 gl_Position;
        float gl_PointSize;
};

out block
{
        vec3 Position;
} Out;

void main()
{
    vec3 pos = Position;
    gl_Position = MVP * vec4(pos, 1.0);
    gl_PointSize = 30.0;

    Out.Position = Position;
}
