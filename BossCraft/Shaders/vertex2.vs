#version 330 core
layout (location = 0) in uint aVertData;

out vec2 TexCoord;
out float ColorMix;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

vec2 cube_uvs[4] = vec2[4](
    vec2(0.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(1.0f, 0.0f),
    vec2(0.0f, 0.0f)
);

float mixVals[4] = float[4](
    0.0f,
    0.1f,
    0.2f,
    0.3f
);

void main()
{
    float x = float(aVertData & 0x1Fu); // 5 bits
    float y = float((aVertData & 0x3FE0u) >> 5u); // 9 bits
    float z = float((aVertData & 0x7C000u) >> 14u); // 5 bits
    uint texIdx = (aVertData & 0x180000u) >> 19u; // 2 bits = index up to 3
    float texU = float((aVertData & 0x3E00000u) >> 21u); // 5 bits = atlas width up to 32
    float texV = float((aVertData & 0x3C000000u) >> 26u); // 4 bits = atlas height up to 16
    uint mixIdx = (aVertData & 0xC0000000u) >> 30u; // 2 bits

    gl_Position = projection * view * model * vec4(x, y, z, 1.0);
    TexCoord = (vec2(cube_uvs[texIdx]) + vec2(texU, texV)) * vec2((1.f / 32.f), (1.f / 16.f));
    ColorMix = mixVals[mixIdx];
}
