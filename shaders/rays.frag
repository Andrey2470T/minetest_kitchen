#version 130

#define MAX_LIGHTS_COUNT 10
uniform sampler2D mBaseTex;
uniform sampler2D mLightTex;
uniform vec4 mLightColor;
//uniform vec2 mLightScreenPos;

varying vec2 vTexCoord;
varying vec2 vLightPos;
//varying vec2 vLightsPos[MAX_LIGHTS_COUNT];

void main(void)
{
    int samples = 128;
    float intensity = 0.125;
    float decay = 0.96875;

    vec2 uv = vTexCoord;
    vec2 dirToCentre = (vLightPos - uv) / float(samples);

    vec3 color = texture2D(mLightTex, uv).rgb;

    for (int i = 0; i < samples; i++)
    {
        color += texture2D(mLightTex, uv).rgb * intensity;
        intensity *= decay;
        uv += dirToCentre;
    }

    vec3 base_color = texture2D(mBaseTex, vTexCoord).rgb;

    float luminance = dot(color*255.0, vec3(0.3, 0.59, 0.11));
    float luminance_orig = dot(vec3(mLightColor)*255.0, vec3(0.3, 0.59, 0.11));

    float ratio = luminance / luminance_orig;
    color = (1.0-ratio)*base_color + ratio*color;

    gl_FragColor = vec4(color, 1.0);
}
