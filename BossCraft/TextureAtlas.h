#pragma once
class TextureAtlas
{
public:
	unsigned int _textureID;
	int _width;
	int _height;
	int _spriteWidth;
	int _spriteHeight;
	float _uStep;
	float _vStep;
	
	/* Loads a .png ONLY */
	TextureAtlas(const char* pathToTexture, int spriteWidth, int spriteHeight);
};

