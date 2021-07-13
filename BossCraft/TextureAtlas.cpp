#include "TextureAtlas.h"

#include <iostream>
#include <ostream>
#include <glad/glad.h>

#include "stb_image.h"

TextureAtlas::TextureAtlas(const char* pathToTexture, int spriteWidth, int spriteHeight)
{	
	glGenTextures(1, &_textureID);
	glBindTexture(GL_TEXTURE_2D, _textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int nrChannels;
	unsigned char* data = stbi_load(pathToTexture, &_width, &_height, &nrChannels, 0);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	_spriteWidth = spriteWidth;
	_spriteHeight = spriteHeight;
	_uStep = static_cast<float>(_spriteWidth) / static_cast<float>(_width);
	_vStep = static_cast<float>(_spriteHeight) / static_cast<float>(_height);
}
