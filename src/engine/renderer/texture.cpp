#include "stdafx.h"
#include "engine/renderer/texture.h"
#include "core/logger.h"

Texture::~Texture() noexcept
{
	if (m_texture != 0)
	{
		glDeleteTextures(1, &m_texture);
	}
}

Texture::Texture(Texture&& other) noexcept
	: m_texture(other.m_texture)
	, m_width(other.m_width)
	, m_height(other.m_height)
{
	other.m_texture = 0;
	other.m_width = 0;
	other.m_height = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
	if (this != &other)
	{
		if (m_texture != 0) glDeleteTextures(1, &m_texture);
		m_texture = other.m_texture;
		m_width = other.m_width;
		m_height = other.m_height;
		other.m_texture = 0;
		other.m_width = 0;
		other.m_height = 0;
	}
	return *this;
}

void Texture::CreateCheckerboard(
	int32_t size,
	int32_t numSquares,
	uint8_t r1, uint8_t g1, uint8_t b1, uint8_t a1,
	uint8_t r2, uint8_t g2, uint8_t b2, uint8_t a2
)
{
	m_width = size;
	m_height = size;

	assert(numSquares > 0 && "numSquares must be positive");

	const int32_t squareSize = size / numSquares;
	std::vector<uint8_t> pixels(size * size * 4);

	for (int32_t y = 0; y < size; ++y)
	{
		for (int32_t x = 0; x < size; ++x)
		{
			int32_t squareY = y / squareSize;
			int32_t squareX = x / squareSize;
			const bool useColor1 = (squareX + squareY) % 2 == 0;

			uint8_t r = useColor1 ? r1 : r2;
			uint8_t g = useColor1 ? g1 : g2;
			uint8_t b = useColor1 ? b1 : b2;
			uint8_t a = useColor1 ? a1 : a2;

			const int32_t idx = (y * size + x) * 4;
			pixels[idx + 0] = r;
			pixels[idx + 1] = g;
			pixels[idx + 2] = b;
			pixels[idx + 3] = a;
		}
	}

	if (m_texture != 0)
	{
		glDeleteTextures(1, &m_texture);
	}
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::applyTextureParams(bool useMipmap) noexcept
{
	if (useMipmap)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

bool Texture::LoadFromFile(std::string_view path, bool useMipmap)
{
	int32_t w = 0, h = 0, channels = 0;
	stbi_uc* data = stbi_load(path.data(), &w, &h, &channels, 4);
	if (!data)
	{
		Logger::Warn(std::string("Texture: failed to load: ") + path.data());
		return false;
	}

	m_width = w;
	m_height = h;

	if (m_texture != 0)
	{
		glDeleteTextures(1, &m_texture);
	}
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	applyTextureParams(useMipmap);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);
	Logger::Info(std::string("Texture loaded: ") + path.data());
	return true;
}

bool Texture::LoadFromMemory(const void* data, size_t size)
{
	int32_t w = 0, h = 0, channels = 0;
	stbi_uc* pixels = stbi_load_from_memory(
		static_cast<const stbi_uc*>(data),
		static_cast<int32_t>(size),
		&w, &h, &channels, 4);
	if (!pixels)
	{
		Logger::Warn("Texture: failed to load from memory");
		return false;
	}

	m_width = w;
	m_height = h;

	if (m_texture != 0)
	{
		glDeleteTextures(1, &m_texture);
	}
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	applyTextureParams(true);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(pixels);
	return true;
}

void Texture::Bind(int32_t unit) const noexcept
{
	assert(unit < 32 && "Texture unit exceeds minimum guaranteed limit");
	if (unit >= 32) [[unlikely]]
	{
		return;
	}
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, m_texture);
}

void Texture::Unbind() noexcept
{
	glBindTexture(GL_TEXTURE_2D, 0);
}
