#version 130

uniform mat4 mModel;
uniform mat4 mView;
uniform mat4 mProjection;

varying vec3 vPos;
varying vec3 vNormal;

void main(void)
{
    vPos = vec3(mModel * gl_Vertex);
    vNormal = vec3(gl_Normal);

    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_Position = mProjection * mView * mModel * gl_Vertex;
}
