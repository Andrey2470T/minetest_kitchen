#version 130

#define MAX_LIGHTS_COUNT 10

uniform sampler2D mBaseTex;
uniform sampler2D mNormalTex;
uniform samplerCube mDepthTex;

uniform vec3 mViewPos;
uniform int mLightsCount;
uniform float mFarPlane;
uniform vec3 mLightsPositions[MAX_LIGHTS_COUNT];
uniform vec4 mLightsColors[MAX_LIGHTS_COUNT];
//uniform float mLightsIntensities[MAX_LIGHTS_COUNT];
uniform vec3 mLightsBBoxMinEdges[MAX_LIGHTS_COUNT];
uniform vec3 mLightsBBoxMaxEdges[MAX_LIGHTS_COUNT];
uniform float mRoughness;
uniform float mMetallic;
uniform float mAO;

varying vec3 vPos;
varying vec3 vNormal;

const float PI = 3.14159265359;
vec4 baseColor = texture2D(mBaseTex, gl_TexCoord[0].st);

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

vec3 Radiance(vec3 lightPos, vec3 lightColor)
{
    float constant = 1.0;
    float linear = 0.35;
    float quadratic = 0.44;

    float distance = length(lightPos - vPos);
    float attenuation = 1.0 / (distance * distance);

    return lightColor;
}

float NormalDistributionGGX(vec3 normal, vec3 midVec, float roughness)
{
    float NdotMV = max(dot(normal, midVec), 0.0);
    float r2 = roughness * roughness;
    float denom = NdotMV * NdotMV * (r2 - 1.0) + 1.0;

    return r2 / (PI * denom * denom);
}

float GeometrySchlickGGX(vec3 normal, vec3 view, float roughness)
{
    float r = roughness + 1.0;
    float k = r * r / 8.0;
    float NdotV = max(dot(normal, view), 0.0);

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 normal, vec3 lightDir, vec3 view, float roughness)
{
    return GeometrySchlickGGX(normal, lightDir, roughness) * GeometrySchlickGGX(normal, view, roughness);
}

vec3 FresnelSchlick(vec3 midVec, vec3 view, vec3 f0)
{
    float MVdotH = max(dot(midVec, view), 0.0);

    return f0 + (1 - f0) * pow(1.0 - MVdotH, 5.0);
}

vec3 calcReflectCapability(vec3 N, vec3 lP, vec3 V, float r, vec3 f0, vec3 lC)
{
    vec3 radiance = Radiance(lP, lC);

    vec3 L = normalize(lP - vPos);
    vec3 H = normalize(V + L);

    float distr = NormalDistributionGGX(N, H, r);
    float geom = GeometrySmith(N, L, V, r);
    vec3 fresnel = FresnelSchlick(H, V, f0);

    vec3 kS = fresnel;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - mMetallic);

    float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);

    vec3 spec = distr * geom * fresnel / max(denom, 0.001);
    vec3 diffuse = vec3(baseColor) / PI;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * diffuse + spec) * radiance * NdotL;
}

vec3 transMapNormalFromTangentToWorldSpace(vec3 pos, vec3 normal, vec2 texCoords)
{
    vec3 q0 = dFdx(pos);
    vec3 q1 = dFdy(pos);

    vec2 st0 = dFdx(texCoords);
    vec2 st1 = dFdy(texCoords);

    vec3 S = normalize(q0 * st1.t - q1 * st0.t);
    vec3 T = normalize(-q0 * st1.s + q1 * st0.s);

    vec3 N = cross(S, T);

    if (dot(N, normal) < 0.0)
    {
        S *= -1.0;
        T *= -1.0;
    }

    mat3 STN = mat3(S, T, N);

    vec3 mapN = texture2D(mNormalTex, texCoords).xyz * 2.0 - 1.0;

    return normalize(STN * mapN);
}

float fragInShadow(vec3 pos, vec3 normal)
{
    float shadow = 0.0;
    vec3 dir = mLightsPositions[0] - pos;
    float bias = max(0.1 * (1.0 - dot(normalize(dir), normal)), 0.1);
    float offset = 0.05;
    int samples = 20;

    float currentDepth = length(dir);

    for (int i = 0; i < samples; i++)
    {
        float closestDepth = textureCube(mDepthTex, dir + sampleOffsetDirections[i] * offset).r;
        closestDepth *= mFarPlane;

        shadow += (currentDepth - bias > closestDepth ? 1.0 : 0.0);
    }

    shadow /= float(samples);

    return shadow;
    /*vec3 dir = mLightsPositions[0] - pos;
    float closestDist = textureCube(mDepthTex, dir).r;
    closestDist *= mFarPlane;

    float currentDist = length(dir);

    float bias = max(0.1 * (1.0-dot(normalize(dir), normal)), 0.1);

    return currentDist - bias > closestDist ? 1.0 : 0.0;*/
    //return closestDist;
}

void main(void)
{
    vec3 viewDir = normalize(mViewPos - vPos);

    vec3 f0 = vec3(0.04);
    f0 = mix(f0, vec3(baseColor), mMetallic);

    vec3 r = vec3(0.0);

    vec3 mapNormal = transMapNormalFromTangentToWorldSpace(vPos, vNormal, gl_TexCoord[0].st);

    for (int i = 0; i < mLightsCount; i++)
    {
        vec3 vertices[8];
        vertices[0] = mLightsBBoxMinEdges[i];
        vertices[1] = vec3(mLightsBBoxMaxEdges[i].x, mLightsBBoxMinEdges[i].y, mLightsBBoxMinEdges[i].z);
        vertices[2] = vec3(mLightsBBoxMaxEdges[i].x, mLightsBBoxMinEdges[i].y, mLightsBBoxMaxEdges[i].z);
        vertices[3] = vec3(mLightsBBoxMinEdges[i].x, mLightsBBoxMinEdges[i].y, mLightsBBoxMaxEdges[i].z);
        vertices[4] = vec3(mLightsBBoxMinEdges[i].x, mLightsBBoxMaxEdges[i].y, mLightsBBoxMinEdges[i].z);
        vertices[5] = vec3(mLightsBBoxMaxEdges[i].x, mLightsBBoxMaxEdges[i].y, mLightsBBoxMinEdges[i].z);
        vertices[6] = mLightsBBoxMaxEdges[i];
        vertices[7] = vec3(mLightsBBoxMinEdges[i].x, mLightsBBoxMaxEdges[i].y, mLightsBBoxMaxEdges[i].z);

        for (int k = 0; k < 8; k++)
            r += calcReflectCapability(vNormal, mLightsPositions[i]+vertices[k], viewDir, mRoughness, f0, vec3(mLightsColors[i]));
    }

    vec3 ambient = vec3(0.1) * vec3(baseColor) * mAO;

    float inShadow = fragInShadow(vPos, vNormal);
    //float k = inShadow == 1.0 ? 0.3 : 1.0;
    vec3 color = ambient + r * (1.0 - inShadow);

    color /= color + vec3(1.0);
    color = pow(color, vec3(1.0/2.2));

    gl_FragColor = vec4(color, baseColor.a);
}
