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
    vec2(0.0f, 0.0f),
);

float mix[4] = float[4](
    0.0f,
    0.1f,
    0.2f,
    0.3f
);

void main()
{
    float x = float(aVertData & 0xFu); // 4 bits
    float y = float((aVertData & 0xFF0u) >> 4u); // 8 bits
    float z = float((aVertData & 0xF000u) >> 12u); // 4 bits
    uint texIdx = (aVertData & 0xFF0000) >> 16u; // 8 bits
    uint mixIdx = (aVertData & 0xF000000) >> 24u; // 4 bits

    gl_Position = projection * view * model * vec4(x, y, z, 1.0);
    TexCoord = cube_uvs[texIdx];
    ColorMix = mix[mixIdx];
}
