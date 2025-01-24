#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../../Util.hpp"

struct ColorAttachment {
    const char* name = "undefined";
    GLuint handle = 0;
    GLenum internalFormat = GL_RGBA;
};
struct DepthAttachment {
    GLuint handle = 0;
    GLenum internalFormat = GL_RGBA;
};

struct GLFrameBuffer {

private:
    const char* name = "undefined";
    GLuint handle = 0;
    GLuint width = 0;
    GLuint height = 0;
    std::vector<ColorAttachment> colorAttachments;
    DepthAttachment depthAttachment;

public:

    void Create(const char* name, int width, int height) {
        glGenFramebuffers(1, &handle);
        this->name = name;
        this->width = width;
        this->height = height;
    }

    void CleanUp() {
        colorAttachments.clear();
        glDeleteFramebuffers(1, &handle);
    }

    void CreateAttachment(const char* name, GLenum internalFormat) {
        GLenum slot = GL_COLOR_ATTACHMENT0 + colorAttachments.size();
        ColorAttachment& colorAttachment = colorAttachments.emplace_back();
        colorAttachment.name = name;
        colorAttachment.internalFormat = internalFormat;
        glBindFramebuffer(GL_FRAMEBUFFER, handle);
        glGenTextures(1, &colorAttachment.handle);
        glBindTexture(GL_TEXTURE_2D, colorAttachment.handle);
        glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, slot, GL_TEXTURE_2D, colorAttachment.handle, 0);
    }

    GLenum GetDepthAttachmentTypeFromDepthFromat(GLenum internalFormat) {
        switch (internalFormat) {
        case GL_DEPTH24_STENCIL8:
        case GL_DEPTH32F_STENCIL8:
            return GL_DEPTH_STENCIL_ATTACHMENT;
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
        case GL_DEPTH_COMPONENT32F:
            return GL_DEPTH_ATTACHMENT;
        default:
            return 0;
        }
    }

    void CreateDepthAttachment(GLenum internalFormat) {
        depthAttachment.internalFormat = internalFormat;
        glBindFramebuffer(GL_FRAMEBUFFER, handle);
        glGenTextures(1, &depthAttachment.handle);
        glBindTexture(GL_TEXTURE_2D, depthAttachment.handle);
        glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GetDepthAttachmentTypeFromDepthFromat(internalFormat), GL_TEXTURE_2D, depthAttachment.handle, 0);
    }

    void Bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, handle);
    }

    void BindExternalDepthBuffer(GLuint textureHandle) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, textureHandle, 0);
    }

    void UnbindDepthBuffer() {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    }

    void DrawBuffers(std::vector<const char*> attachmentNames) {
        std::vector<GLuint> attachments;
        for (const char* attachmentName : attachmentNames) {
            attachments.push_back(GetColorAttachmentSlotByName(attachmentName));
        }
        glDrawBuffers(attachments.size(), &attachments[0]);
    }

    void DrawBuffer(const char* attachmentName) {
        for (int i = 0; i < colorAttachments.size(); i++) {
            if (Util::StrCmp(attachmentName, colorAttachments[i].name)) {
                glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
                return;
            }
        }
    }

    void SetViewport() {
        glViewport(0, 0, width, height);
    }

    GLuint GetHandle() {
        return handle;
    }

    GLuint GetWidth() {
        return width;
    }

    GLuint GetHeight() {
        return height;
    }

    GLuint GetColorAttachmentHandleByName(const char* name) {
        for (int i = 0; i < colorAttachments.size(); i++) {
            if (Util::StrCmp(name, colorAttachments[i].name)) {
                return colorAttachments[i].handle;
            }
        }
        std::cout << "GetColorAttachmentHandleByName() with name '" << name << "' failed. Name does not exist in FrameBuffer '" << this->name << "'\n";
        return GL_NONE;
    }

    GLuint GetDepthAttachmentHandle() {
        return depthAttachment.handle;
    }

    GLenum GetColorAttachmentSlotByName(const char* name) {
        for (int i = 0; i < colorAttachments.size(); i++) {
            if (Util::StrCmp(name, colorAttachments[i].name)) {
                return GL_COLOR_ATTACHMENT0 + i;
            }
        }
        std::cout << "GetColorAttachmentHandleByName() with name '" << name << "' failed. Name does not exist in FrameBuffer '" << this->name << "'\n";
        return GL_INVALID_VALUE;
    }
    
    void ClearAttachment(const char* attachmentName, float r, float g, float b, float a) {
        for (int i = 0; i < colorAttachments.size(); i++) {
            if (Util::StrCmp(attachmentName, colorAttachments[i].name)) {
                glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
                glClearColor(r, g, b, a);
                glClear(GL_COLOR_BUFFER_BIT);
                glDrawBuffer(GL_NONE);
                return;
            }
        }
    }

    void ClearDepthAttachment() {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
};