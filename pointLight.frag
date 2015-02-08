#version 410 core
in block
{
    vec2 Texcoord;
} In;
uniform sampler2D ColorBuffer;
uniform sampler2D NormalBuffer;
uniform sampler2D DepthBuffer;
uniform mat4 ScreenToWorld;
uniform vec3 pointLightPosition;
uniform vec3 pointLightColor;
uniform float pointLightIntensity;
uniform vec3 Camera;

layout(location = 0) out vec4 Color;

vec3 illuminationPointLight(vec3 positionLight, vec3 positionObject, vec3 color, float intensity, vec3 diffuseColor,
                            vec3 specular, float specularPower, vec3 normal)
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

    //TODO faire une specular intensity
    return ((diffuseColor * ndotl * color * intensity) + (specularColor * intensity)) * attenuation;
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

    vec3 pointLight1 = illuminationPointLight(pointLightPosition, p, pointLightColor, pointLightIntensity, diffuseColor,
                                              specularColor, specularPower, n );

    Color = vec4(pointLight1, 1.0);
}
