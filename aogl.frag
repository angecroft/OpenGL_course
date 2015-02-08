#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp int;

uniform sampler2D Diffuse;
uniform sampler2D Diffuse2;
uniform vec3 Light;

//uniform vec3 pointLightPosition[4];
//uniform vec3 pointLightColor[4];
//uniform float pointLightIntensity[4];

//uniform float spotAngle;
//uniform vec3 spotDirection;

uniform vec3 Camera;
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


vec3 illuminationDirectionalLight(vec3 direction, vec3 color, float intensity, vec3 diffuseColor)
{
    // directional
    vec3 l = normalize(direction);
    float ndotl = clamp(dot(In.Normal, l), 0.0, 1.0);

    // specularPower
    vec3 spec = texture(Diffuse2, In.TexCoord).rgb;
    vec3 v = normalize(Camera - In.Position);
    vec3 h = normalize(l+v);
    float ndoth = clamp(dot(In.Normal, h), 0.0, 1.0);
    vec3 specularColor =  spec * pow(ndoth, specularPower);

    // Specular est en N&B donc chaque composante a la même valeur
    Color.a = spec.r;

    return (diffuseColor * ndotl * color * intensity) + specularColor * intensity;
}

void main()
{

    vec3 diffuseColor = texture(Diffuse, In.TexCoord).rgb;
    vec3 spec = texture(Diffuse2, In.TexCoord).rgb;

    // illumination diffuse
    vec3 l = normalize(Light - In.Position);
    // Directional Light
  //  vec3 directionalLightIllumination = illuminationDirectionalLight(pointLightPosition[2], pointLightColor[2], pointLightIntensity[2], diffuseColor);

//    FragColor = vec4(directionalLightIllumination,  1);

    Normal.rgb = In.Normal;
    Normal.a = specularPower;

    Color.rgb = diffuseColor;
    Color.a = spec.r;

}
