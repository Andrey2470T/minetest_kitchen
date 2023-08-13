#version 130

uniform sampler2D mBaseTex;
uniform sampler2D mRaysTex1;
uniform sampler2D mRaysTex2;
uniform sampler2D mRaysTex3;
uniform sampler2D mRaysTex4;
uniform sampler2D mRaysTex5;
uniform sampler2D mRaysTex6;
uniform sampler2D mRaysTex7;
uniform sampler2D mRaysTex8;
uniform sampler2D mRaysTex9;
uniform sampler2D mRaysTex10;

varying vec2 vTexCoord;

void main(void)
{
    vec4 base_color = texture2D(mBaseTex, vTexCoord);
    vec4 rays_color1 = texture2D(mRaysTex1, vTexCoord);
    vec4 rays_color2 = texture2D(mRaysTex2, vTexCoord);
    vec4 rays_color3 = texture2D(mRaysTex3, vTexCoord);
    vec4 rays_color4 = texture2D(mRaysTex4, vTexCoord);
    vec4 rays_color5 = texture2D(mRaysTex5, vTexCoord);
    vec4 rays_color6 = texture2D(mRaysTex6, vTexCoord);
    vec4 rays_color7 = texture2D(mRaysTex7, vTexCoord);
    vec4 rays_color8 = texture2D(mRaysTex8, vTexCoord);
    vec4 rays_color9 = texture2D(mRaysTex9, vTexCoord);
    vec4 rays_color10 = texture2D(mRaysTex10, vTexCoord);

    vec4 color = base_color + rays_color1 + rays_color2 + rays_color3 + rays_color4 + rays_color5 +
            rays_color6 + rays_color7 + rays_color8 + rays_color9 + rays_color10;

    gl_FragColor = vec4(vec3(color), 1.0);
}
