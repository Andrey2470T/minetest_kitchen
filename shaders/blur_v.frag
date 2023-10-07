#version 130
uniform sampler2D mTex;
varying vec2 vTexCoord;

const int blurRadius = 30;
const float tex_height = 1.0/480.0;
float gauss_nums[blurRadius];

highp float GaussDistrFunc(int x)
{
    float pi = 3.1415927;
    float e = 2.71828;
    float theta = 0.1;

    return 1.0 / (theta * sqrt(2 * pi)) * pow(e, -(x * x) / 2 * theta * theta);
}

void main(void)
{
    float sum = 0.0;

    for (int i = 0; i < blurRadius; i++)
    {
        gauss_nums[i] = GaussDistrFunc(i);
        sum += gauss_nums[i];
    }

    for (int i = 0; i < blurRadius; i++)
        gauss_nums[i] /= sum;

    vec4 blurColor = texture2D(mTex, vec2(vTexCoord)) * gauss_nums[0];
    for (int i = 1; i < blurRadius; i++)
    {
        vec2 left_texcoord = vec2(vTexCoord.x, vTexCoord.y - tex_height * i);
        vec2 right_texcoord = vec2(vTexCoord.x, vTexCoord.y + tex_height * i);

        if (left_texcoord.y >= 0.0)
            blurColor += texture2D(mTex, left_texcoord) * gauss_nums[i];
        if (right_texcoord.y <= 1.0)
            blurColor += texture2D(mTex, right_texcoord) * gauss_nums[i];
    }

    gl_FragColor = blurColor;
}
