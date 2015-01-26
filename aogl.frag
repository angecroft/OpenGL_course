#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp int;

uniform sampler2D Diffuse;
uniform sampler2D Diffuse2;
uniform vec3 Light;
uniform vec3 Camera;
uniform int specularPower;

layout(location = FRAG_COLOR, index = 0) out vec4 FragColor;

in block
{
	vec2 TexCoord;
        vec3 Position;
        vec3 Normal;
        float Time;
} In;

void main()
{
//     FragColor = vec4(In.Normal, 0);
     //FragColor = vec4(vec3(In.Position), 0);

    vec3 diffuseColor = texture(Diffuse, In.TexCoord).rgb;
    vec3 spec = texture(Diffuse2, In.TexCoord).rgb;

    // illumination

    vec3 l = normalize(Light - In.Position);
//    float ndotl =  dot(In.Normal, l);
//    vec3 color = mix(diffuse, diffuse2, 0.5) * ndotl;
    float ndotl =  clamp(dot(In.Normal, l), 0.0, 1.0);
    vec3 color = diffuseColor * ndotl;

    // BlinnPhong
    vec3 e = normalize(Camera - In.Position);
//    vec3 h = normalize(l-e);
//    float ndoth = dot(In.Normal, h);
//    vec3 specularColor =  diffuse2 * pow(ndoth, specularPower);

    vec3 h = normalize(l-e);
    float ndoth = clamp(dot(In.Normal, h), 0.0, 1.0);
    vec3 specularColor =  spec * pow(ndoth, specularPower);

//    vec2 tex = vec2(abs(cos(In.TexCoord.x * 10)), abs(sin(In.TexCoord.y * 10)));
//    float ring = 1.0 - pow(abs(cos(In.Time)), tex.x) + pow(0.7, tex.y);

//    FragColor = vec4(ring*0.4*abs(sin(In.Time)), ring*0.56, ring*.8, 1);

//    FragColor = vec4(diffuseColor, 1);

//    FragColor = vec4(color , 1); // diffuse light
  FragColor = vec4(specularColor, 1);

}
