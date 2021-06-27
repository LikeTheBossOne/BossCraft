#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;
uniform float redColor;
uniform float greenColor;

void main()
{
    FragColor = vec4(redColor, greenColor, .5, 0.1f); //* texture(texture1, TexCoord);
}