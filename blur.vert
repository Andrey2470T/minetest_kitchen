#version 130
varying vec2 vTexCoord;

void main(void)
{
    vec2 Pos = sign(gl_Vertex.xy); // Clean up inaccuracies
    gl_Position = vec4(Pos, 0.0, 1.0);
    vTexCoord = Pos * 0.5 + 0.5; // Image-space
}
