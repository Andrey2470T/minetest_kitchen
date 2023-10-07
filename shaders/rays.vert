#version 130

#define MAX_LIGHTS_COUNT 10
uniform mat4 mModel;
uniform mat4 mView;
uniform mat4 mProjection;

//uniform int mLightsCount;
uniform vec3 mLightPos;
//uniform vec3 mLightsPos[MAX_LIGHTS_COUNT];

varying vec2 vTexCoord;
varying vec2 vLightPos;
//varying vec2 vLightsPos[MAX_LIGHTS_COUNT];

void main(void)
{
    vec2 Pos = sign(gl_Vertex.xy); // Clean up inaccuracies
    gl_Position = vec4(Pos, 0.0, 1.0);
    vTexCoord = Pos * 0.5 + 0.5; // Image-space

    vec4 lightPos = mProjection * mView * vec4(mLightPos, 1.0);
    lightPos /= lightPos.w;
    vLightPos = vec2(lightPos.x, lightPos.y);
    vLightPos = vLightPos * 0.5 + 0.5;
}
