#pragma once

class Texture final
{
public:
	Texture() noexcept = default;
	~Texture() noexcept;

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	void CreateCheckerboard(
		int32_t size,
		int32_t numSquares,
		uint8_t r1, uint8_t g1, uint8_t b1, uint8_t a1,
		uint8_t r2, uint8_t g2, uint8_t b2, uint8_t a2
	);

	[[nodiscard]] bool LoadFromFile(std::string_view path, bool useMipmap = true);
	[[nodiscard]] bool LoadFromMemory(const void* data, size_t size);

	// Create a raw RGBA8 texture from pixel data (no mipmaps, nearest filter)
	void CreateFromRaw(int32_t w, int32_t h, const uint32_t* pixels);

	void Bind(int32_t unit = 0) const noexcept;
	static void Unbind() noexcept;

	[[nodiscard]] uint32_t Handle() const noexcept
	{
		return m_texture;
	}

	[[nodiscard]] int32_t Width() const noexcept
	{
		return m_width;
	}

	[[nodiscard]] int32_t Height() const noexcept
	{
		return m_height;
	}

	[[nodiscard]] explicit operator bool() const noexcept
	{
		return m_texture != 0;
	}

private:
	static void applyTextureParams(bool useMipmap) noexcept;

	uint32_t m_texture = 0;
	int32_t m_width = 0;
	int32_t m_height = 0;
};
