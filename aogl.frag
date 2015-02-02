#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp int;

uniform sampler2D Diffuse;
uniform sampler2D Diffuse2;
uniform vec3 Light;

uniform vec3 pointLightPosition[4];
uniform vec3 pointLightColor[4];
uniform float pointLightIntensity[4];

uniform float spotAngle;
uniform vec3 spotDirection;

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

vec3 illuminationPointLight(vec3 position, vec3 color, float intensity, vec3 diffuseColor)
{
    // point light
    vec3 l = normalize(position - In.Position);
    float ndotl = clamp(dot(In.Normal, l), 0.0, 1.0);

    // specular
    vec3 spec = texture(Diffuse2, In.TexCoord).rgb;
    vec3 v = normalize(Camera - In.Position);
    vec3 h = normalize(l+v);
    float ndoth = clamp(dot(In.Normal, h), 0.0, 1.0);
    vec3 specularColor =  spec * pow(ndoth, specularPower);

    // attenuation
    float distance = length(position - In.Position);
    float attenuation = 1.0/ (pow(distance,2)*.1 + 1.0);

    //TODO faire une specular intensity
    return ((diffuseColor * ndotl * color * intensity) + (specularColor * intensity)) * attenuation;
}

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

    return (diffuseColor * ndotl * color * intensity) + specularColor * intensity;
}

vec3 illuminationSpotLight(vec3 direction, vec3 position, vec3 color, float angle, float intensity, vec3 diffuseColor)
{
    // point light
    vec3 l = normalize(position - In.Position);
    float ndotl = clamp(dot(In.Normal, l), 0.0, 1.0);

    // specular
    vec3 spec = texture(Diffuse2, In.TexCoord).rgb;
    vec3 v = normalize(Camera - In.Position);
    vec3 h = normalize(l+v);
    float ndoth = clamp(dot(In.Normal, h), 0.0, 1.0);
    vec3 specularColor =  spec * pow(ndoth, specularPower);

    // attenuation
    float distance = length(position - In.Position);
    float attenuation = 1.0/ (pow(distance,2)*.1 + 1.0);

    angle = radians(angle);
    float theta = acos(dot(-l, normalize(direction)));

    if(theta > angle / 2)
        return vec3(0);

    float falloff = pow(((cos(theta) - cos(angle)) / (cos(0.1) - cos(angle))), 4);


    return ((diffuseColor * ndotl * color * intensity) + (specularColor * intensity)) * attenuation * falloff;
}

void main()
{

    vec3 diffuseColor = texture(Diffuse, In.TexCoord).rgb;

    // illumination diffuse
    vec3 l = normalize(Light - In.Position);

    // Point light
    vec3 pointLightIllumination1 = illuminationPointLight(pointLightPosition[0], pointLightColor[0], pointLightIntensity[0], diffuseColor);
    vec3 pointLightIllumination2 = illuminationPointLight(pointLightPosition[1], pointLightColor[1], pointLightIntensity[1], diffuseColor);

    // Directional Light
    vec3 directionalLightIllumination = illuminationDirectionalLight(pointLightPosition[2], pointLightColor[2], pointLightIntensity[2], diffuseColor);

    // Spot Light
    vec3 spotLightIllumination = illuminationSpotLight(spotDirection, pointLightPosition[3], pointLightColor[3], spotAngle, pointLightIntensity[3], diffuseColor);

    FragColor = vec4(directionalLightIllumination + pointLightIllumination1 + pointLightIllumination2 + spotLightIllumination, 1);
}
