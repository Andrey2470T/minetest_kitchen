#version 130

uniform float mNear;
uniform float mFar;

varying vec3 vPos;
varying vec3 vNormal;

float linearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * mNear * mFar) / (mFar + mNear - z * (mFar - mNear));
}

void main(void)
{
    gl_FragColor = vec4(gl_FragCoord.z, 0.0, 0.0, 0.0);
}
