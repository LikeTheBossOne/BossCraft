#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in float ColorMix;

uniform sampler2D texture1;

void main()
{
    FragColor = mix(texture(texture1, TexCoord), vec4(0, 0, 0, 1), ColorMix);
}
