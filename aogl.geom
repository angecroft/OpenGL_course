#version 410 core

precision highp float;
precision highp int;
layout(std140, column_major) uniform;
layout(triangles) in;
layout(triangle_strip, max_vertices = 4) out;

in block
{
    vec2 TexCoord;
    vec3 Position;
    vec3 Normal;
} In[];

out block
{
    vec2 TexCoord;
    vec3 Position;
    vec3 Normal;
}Out;

uniform mat4 MVP;
uniform mat4 MV;
uniform float Time;

void main()
{
    for(int i = 0; i < gl_in.length(); ++i)
    {
        if(gl_PrimitiveIDIn == 0) {
     //       gl_Position = vec4(0,0,0,1);
     //       Out.Position = vec3(0,0,0);
     //       Out.Normal = vec3(0,0,0);
        }
        else {
            gl_Position = gl_in[i].gl_Position;
        }
        Out.TexCoord = In[i].TexCoord;
        Out.Position = In[i].Position;
        Out.Normal = In[i].Normal;
        EmitVertex();
    }
    EndPrimitive();
}
