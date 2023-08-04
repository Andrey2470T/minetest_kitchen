#version 130

#define MAX_LIGHTS_COUNT 10
uniform sampler2D mTex;
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

    vec3 color = texture2D(mTex, uv).rgb;

    for (int i = 0; i < samples; i++)
    {
        color += texture2D(mTex, uv).rgb * intensity;
        intensity *= decay;
        uv += dirToCentre;
    }

    gl_FragColor = vec4(color, 1.0);
}
