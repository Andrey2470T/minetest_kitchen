#version 130

uniform sampler2D mDepthTex;
uniform vec2 mScreenSize;
uniform vec4 mLightColor;

varying vec3 vPos;
varying vec3 vNormal;

void main(void)
{
    vec2 depthTexCoords = gl_FragCoord.st / vec2(1024, 680);
    float depth = texture2D(mDepthTex, depthTexCoords).r;

    vec3 color = vec3(0.0);
    int light_id = 0;

    if (gl_FragCoord.z < depth || depth == 0.0)
        color = vec3(mLightColor);

    gl_FragColor = vec4(color, 1.0);
}
