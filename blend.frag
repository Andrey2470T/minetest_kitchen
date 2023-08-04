#version 130

uniform sampler2D mBaseTex;
uniform sampler2D mBlurTex;

varying vec2 vTexCoord;

void main(void)
{
    vec4 color1 = texture2D(mBaseTex, vTexCoord);
    vec4 color2 = texture2D(mBlurTex, vTexCoord);
    vec4 color = color1 + color2;
    //vec3 res = vec3(1.0) - exp(-color);
    //res = pow(res, vec3(1.0/2.2));

    gl_FragColor = vec4(vec3(color), max(color1.a, color2.a));
}
