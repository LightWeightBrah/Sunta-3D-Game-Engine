#include "Core/SuntaPreCompiled.h"
#include "OpenGLFramebuffer.h"

#include <glad/glad.h>

#include "Core/Log.h"
#include "OpenGLUtilities.h"

namespace Sunta
{

namespace
{

bool IsDepth(FramebufferAttachment attachment)
{
	return attachment == FramebufferAttachment::Depth24;
}

unsigned int CreateColorTexture(FramebufferAttachment attachment, int width, int height)
{
	const bool isFloat = attachment == FramebufferAttachment::RGBA16F;

	unsigned int texture = 0;
	GLCall(glGenTextures(1, &texture));
	GLCall(glBindTexture(GL_TEXTURE_2D, texture));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, isFloat ? GL_RGBA16F : GL_RGBA8, width, height, 0, GL_RGBA, isFloat ? GL_FLOAT : GL_UNSIGNED_BYTE, nullptr));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	return texture;
}

// Read outside its edges, the depth is the farthest (for a shadow map: nothing outside it is in shadow)
unsigned int CreateDepthTexture(int width, int height)
{
	unsigned int texture = 0;
	GLCall(glGenTextures(1, &texture));
	GLCall(glBindTexture(GL_TEXTURE_2D, texture));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

	const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
	GLCall(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor));
	return texture;
}

}

OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& specification)
	: specification(specification)
{
	Create();
}

OpenGLFramebuffer::~OpenGLFramebuffer()
{
	Destroy();
}

void OpenGLFramebuffer::Create()
{
	const int width  = specification.width;
	const int height = specification.height;

	GLint previous = 0;
	GLCall(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous));

	GLCall(glGenFramebuffers(1, &id));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, id));

	std::vector<GLenum> drawBuffers;

	for (FramebufferAttachment attachment : specification.attachments)
	{
		if (IsDepth(attachment))
		{
			depthTexture = CreateDepthTexture(width, height);
			GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0));
			continue;
		}

		const GLenum colorAttachment = GL_COLOR_ATTACHMENT0 + (GLenum)colorTextures.size();
		colorTextures.push_back(CreateColorTexture(attachment, width, height));
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachment, GL_TEXTURE_2D, colorTextures.back(), 0));
		drawBuffers.push_back(colorAttachment);
	}

	// Only depth (a shadow map): no color is drawn or read
	if (drawBuffers.empty())
	{
		GLCall(glDrawBuffer(GL_NONE));
		GLCall(glReadBuffer(GL_NONE));
	}
	else
	{
		GLCall(glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data()));
	}

	isComplete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	if (!isComplete)
		SUNTA_ENGINE_LOG_ERROR("Framebuffer: the framebuffer of {0}x{1} with {2} pictures could not be made", width, height, specification.attachments.size());

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, previous));
}

void OpenGLFramebuffer::Destroy()
{
	glDeleteFramebuffers(1, &id);
	glDeleteTextures((GLsizei)colorTextures.size(), colorTextures.data());
	glDeleteTextures(1, &depthTexture);

	id           = 0;
	depthTexture = 0;
	colorTextures.clear();
}

void OpenGLFramebuffer::Bind()
{
	GLCall(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer));
	GLCall(glGetIntegerv(GL_VIEWPORT, previousViewport));

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, id));
	GLCall(glViewport(0, 0, specification.width, specification.height));
}

void OpenGLFramebuffer::Unbind()
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer));
	GLCall(glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]));
}

void OpenGLFramebuffer::Clear()
{
	GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	GLCall(glDepthMask(GL_TRUE));
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void OpenGLFramebuffer::Resize(int width, int height)
{
	if (width == specification.width && height == specification.height)
		return;

	specification.width  = width;
	specification.height = height;

	Destroy();
	Create();
}

void OpenGLFramebuffer::BindColorTexture(unsigned int index, unsigned int slot) const
{
	if (index >= colorTextures.size())
		return;

	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_2D, colorTextures[index]));
}

void OpenGLFramebuffer::BindDepthTexture(unsigned int slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_2D, depthTexture));
}

}
