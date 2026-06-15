#pragma once

class FrameBuffer final
{
public:
	FrameBuffer(uint32_t width, uint32_t height);
	~FrameBuffer() noexcept;

	FrameBuffer(const FrameBuffer&) = delete;
	FrameBuffer& operator=(const FrameBuffer&) = delete;
	FrameBuffer(FrameBuffer&& other) noexcept;
	FrameBuffer& operator=(FrameBuffer&& other) noexcept;

	void Bind() noexcept;
	void Unbind() noexcept;

	void Resize(uint32_t width, uint32_t height);

	[[nodiscard]] uint32_t Width() const noexcept
	{
		return m_width;
	}

	[[nodiscard]] uint32_t Height() const noexcept
	{
		return m_height;
	}

	[[nodiscard]] uint32_t ColorTexture() const noexcept
	{
		return m_colorTexture;
	}

private:
	void create();
	void destroy() noexcept;

	uint32_t m_fbo = 0;
	uint32_t m_colorTexture = 0;
	uint32_t m_depthRbo = 0;
	uint32_t m_width = 0;
	uint32_t m_height = 0;
};
