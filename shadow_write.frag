#version 130

uniform vec3 mLightPos;
uniform float mFarPlane;

varying vec3 vPos;
varying vec3 vNormal;

void main(void)
{
    float dist = length(vPos - mLightPos) / mFarPlane;

    gl_FragColor = vec4(vec3(dist), 1.0);
}
