#include "stdafx.h"
#include "engine/renderer/framebuffer.h"

// Platform-dependent depth format:
//   - Desktop (OpenGL 3.3+): 24-bit depth + 8-bit stencil for future use
//   - WebGL2  (emscripten):   24-bit depth only (simpler, wider compatibility)
#if defined(__EMSCRIPTEN__)
    constexpr GLenum DEPTH_RBO_FORMAT = GL_DEPTH_COMPONENT24;
    constexpr GLenum DEPTH_ATTACH_PT  = GL_DEPTH_ATTACHMENT;
#else
    constexpr GLenum DEPTH_RBO_FORMAT = GL_DEPTH24_STENCIL8;
    constexpr GLenum DEPTH_ATTACH_PT  = GL_DEPTH_STENCIL_ATTACHMENT;
#endif

// ── FrameBuffer implementation ────────────────────────────────────────────

FrameBuffer::FrameBuffer(uint32_t width, uint32_t height)
	: m_width(width)
	, m_height(height)
{
	create();
}

FrameBuffer::~FrameBuffer() noexcept
{
	destroy();
}

FrameBuffer::FrameBuffer(FrameBuffer&& other) noexcept
	: m_fbo(other.m_fbo)
	, m_colorTexture(other.m_colorTexture)
	, m_depthRbo(other.m_depthRbo)
	, m_width(other.m_width)
	, m_height(other.m_height)
{
	other.m_fbo = 0;
	other.m_colorTexture = 0;
	other.m_depthRbo = 0;
	other.m_width = 0;
	other.m_height = 0;
}

FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) noexcept
{
	if (this != &other)
	{
		destroy();
		m_fbo = other.m_fbo;
		m_colorTexture = other.m_colorTexture;
		m_depthRbo = other.m_depthRbo;
		m_width = other.m_width;
		m_height = other.m_height;
		other.m_fbo = 0;
		other.m_colorTexture = 0;
		other.m_depthRbo = 0;
		other.m_width = 0;
		other.m_height = 0;
	}
	return *this;
}

// ── Private helpers ───────────────────────────────────────────────────────

void FrameBuffer::create()
{
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	// Color attachment
	glGenTextures(1, &m_colorTexture);
	glBindTexture(GL_TEXTURE_2D, m_colorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<int32_t>(m_width), static_cast<int32_t>(m_height), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

	// Depth renderbuffer (platform-dependent format)
	glGenRenderbuffers(1, &m_depthRbo);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
	glRenderbufferStorage(GL_RENDERBUFFER, DEPTH_RBO_FORMAT, static_cast<int32_t>(m_width), static_cast<int32_t>(m_height));
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, DEPTH_ATTACH_PT, GL_RENDERBUFFER, m_depthRbo);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE && "Framebuffer is not complete");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void FrameBuffer::destroy() noexcept
{
	if (m_fbo != 0)
	{
		glDeleteFramebuffers(1, &m_fbo);
		m_fbo = 0;
	}
	if (m_colorTexture != 0)
	{
		glDeleteTextures(1, &m_colorTexture);
		m_colorTexture = 0;
	}
	if (m_depthRbo != 0)
	{
		glDeleteRenderbuffers(1, &m_depthRbo);
		m_depthRbo = 0;
	}
}

void FrameBuffer::Bind() noexcept
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void FrameBuffer::Unbind() noexcept
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::Resize(uint32_t width, uint32_t height)
{
	if (width == m_width && height == m_height)
	{
		return;
	}
	m_width = width;
	m_height = height;

	glBindTexture(GL_TEXTURE_2D, m_colorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<int32_t>(m_width), static_cast<int32_t>(m_height), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
	glRenderbufferStorage(GL_RENDERBUFFER, DEPTH_RBO_FORMAT, static_cast<int32_t>(m_width), static_cast<int32_t>(m_height));
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE && "Resized framebuffer is not complete");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
