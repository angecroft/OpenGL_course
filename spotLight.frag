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

//uniform sampler2D Shadow;
uniform sampler2DShadow Shadow;     // with Percentage Closer Filtering OpenGL
uniform float bias;
uniform mat4 worldToLightScreen;

uniform vec3 Camera;

layout(location = 0) out vec4 Color;

vec3 illuminationSpotLight(vec3 direction, vec3 positionLight, vec3 positionObject, vec3 color, float angle,
                           float intensity, vec3 diffuseColor, vec3 specular, float specularPower, vec3 normal, vec3 lP, float shadowDepth)
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

    if(shadowDepth < lP.z - bias)
        return vec3(0);
    else
        return ((diffuseColor * ndotl * color * intensity) + (specularColor * intensity)) * attenuation * falloff;
}

// Arbitrary Percentage Closer Filtering
vec2 poissonDisk[16] = vec2[](
    vec2( -0.94201624, -0.39906216 ),
    vec2( 0.94558609, -0.76890725 ),
    vec2( -0.094184101, -0.92938870 ),
    vec2( 0.34495938, 0.29387760 ),
    vec2( -0.91588581, 0.45771432 ),
    vec2( -0.81544232, -0.87912464 ),
    vec2( -0.38277543, 0.27676845 ),
    vec2( 0.97484398, 0.75648379 ),
    vec2( 0.44323325, -0.97511554 ),
    vec2( 0.53742981, -0.47373420 ),
    vec2( -0.26496911, -0.41893023 ),
    vec2( 0.79197514, 0.19090188 ),
    vec2( -0.24188840, 0.99706507 ),
    vec2( -0.81409955, 0.91437590 ),
    vec2( 0.19984126, 0.78641367 ),
    vec2( 0.14383161, -0.14100790 )
);
float random(vec4 seed)
{
    float dot_product = dot(seed, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
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
    vec3 p = vec3(wP.xyz / wP.w);

    vec4 wlP = worldToLightScreen * vec4(p, 1.0);
    vec3 lP = vec3(wlP/wlP.w) * 0.5 + 0.5;

    float d = distance(spotLightPosition, p);

//    float shadowDepth = texture(Shadow, lP.xy).r;
//    float shadowDepth = textureProj(Shadow, vec4(lP.xy, lP.z -bias, 1.0), 0.0);

    float shadowDepth = 0.0;
    int SampleCount = 2;
    float samplesf = SampleCount;
    for (int i=0;i<SampleCount;i++)
    {
        int index = int(samplesf*random(vec4(gl_FragCoord.xyy, i)))%SampleCount;
        shadowDepth += textureProj(Shadow, vec4(lP.xy + poissonDisk[index]/(1000.0 * 1.f/d), lP.z -0.005, 1.0), 0.0) / samplesf;
    }

    vec3 spotLight = illuminationSpotLight(spotLightDirection, spotLightPosition, p, spotLightColor, spotLightAngle,
                                           spotLightIntensity, diffuseColor, specularColor, specularPower, n, lP, shadowDepth );

    Color = vec4(spotLight, 1.0);

}
