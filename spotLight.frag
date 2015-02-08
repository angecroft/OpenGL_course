#version 410 core
in block
{
    vec2 Texcoord;
} In;
uniform sampler2D ColorBuffer;
uniform sampler2D NormalBuffer;
uniform sampler2D DepthBuffer;
uniform mat4 ScreenToWorld;

uniform vec3 spotLightPosition;
uniform vec3 spotLightDirection;
uniform vec3 spotLightColor;
uniform float spotLightIntensity;
uniform float spotLightAngle;

uniform vec3 Camera;

layout(location = 0) out vec4 Color;

vec3 illuminationSpotLight(vec3 direction, vec3 positionLight, vec3 positionObject, vec3 color, float angle,
                           float intensity, vec3 diffuseColor, vec3 specular, float specularPower, vec3 normal)
{
    // point light
    vec3 l = normalize(positionLight - positionObject);
    float ndotl = clamp(dot(normal, l), 0.0, 1.0);

    // specular
    vec3 v = normalize(Camera - positionObject);
    vec3 h = normalize(l+v);
    float ndoth = clamp(dot(normal, h), 0.0, 1.0);
    vec3 specularColor =  specular * pow(ndoth, specularPower);

    // attenuation
    float distance = length(positionLight - positionObject);
    float attenuation = 1.0/ (pow(distance,2)*.1 + 1.0);

    angle = radians(angle);
    float theta = acos(dot(-l, normalize(direction)));

    float falloff = clamp(pow(((cos(theta) - cos(angle)) / (cos(angle) - (cos(0.3+angle)))), 4), 0, 1);


    return ((diffuseColor * ndotl * color * intensity) + (specularColor * intensity)) * attenuation * falloff;
}

void main(void)
{
    // Read gbuffer values
    vec4 colorBuffer = texture(ColorBuffer, In.Texcoord).rgba;
    vec4 normalBuffer = texture(NormalBuffer, In.Texcoord).rgba;
    float depth = texture(DepthBuffer, In.Texcoord).r;

    // Unpack values stored in the gbuffer
    vec3 diffuseColor = colorBuffer.rgb;
    vec3 specularColor = colorBuffer.aaa;
    float specularPower = normalBuffer.a;
    vec3 n = normalBuffer.rgb;

    // Convert texture coordinates into screen space coordinates
    vec2 xy = In.Texcoord * 2.0 -1.0;
    // Convert depth to -1,1 range and multiply the point by ScreenToWorld matrix
    vec4 wP = vec4(xy, depth * 2.0 -1.0, 1.0) * ScreenToWorld;
    // Divide by w
    vec3 p = vec3(wP.xyz / wP.w);

    vec3 spotLight = illuminationSpotLight(spotLightDirection, spotLightPosition, p, spotLightColor, spotLightAngle,
                                           spotLightIntensity, diffuseColor, specularColor, specularPower, n );

    Color = vec4(spotLight, 1.0);
}
