#pragma once
#include <glad/glad.h>
#include <cstdint>

struct PBO {
public:
    void Init(size_t size) {
        glGenBuffers(1, &m_handle);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_handle);

        // Allocate buffer storage with persistent mapping flags
        glBufferStorage(GL_PIXEL_UNPACK_BUFFER, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

        // Map the buffer persistently and store the mapped pointer
        m_mappedPointer = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, size,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        if (!m_mappedPointer) {
            std::cerr << "Failed to map PBO persistently!\n";
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            return;
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        m_size = size;
    }

    void Bind() {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_handle);
    }

    void Unbind() {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    uint32_t GetHandle() {
        return m_handle;
    }
    
    void* Map() {
        Bind();
        return glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    }

    void Unmap() {
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        Unbind();
    }

    void Resize(size_t newSize) {
        if (newSize > m_size) {
            // Unmap the current buffer if it was previously mapped
            if (m_mappedPointer) {
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_handle);
                glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
                m_mappedPointer = nullptr;
            }

            // Delete the old buffer
            glDeleteBuffers(1, &m_handle);

            // Reinitialize the buffer with the new size
            glGenBuffers(1, &m_handle);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_handle);

            // Allocate new buffer storage with persistent mapping flags
            glBufferStorage(GL_PIXEL_UNPACK_BUFFER, newSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

            // Map the new buffer persistently
            m_mappedPointer = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, newSize,
                GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
            if (!m_mappedPointer) {
                std::cerr << "Failed to map resized PBO persistently!\n";
            }

            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

            // Update the size
            m_size = newSize;
        }
    }


    void SetSync() {
        if (m_syncObj) {
            glDeleteSync(m_syncObj);
        }
        m_syncObj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    bool IsSyncComplete() {
        if (!m_syncObj) return true;

        GLenum status = glClientWaitSync(m_syncObj, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
        if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
            glDeleteSync(m_syncObj);
            m_syncObj = nullptr;
            return true;
        }
        return false;
    }

    void CleanUp() {
        if (m_mappedPointer) {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_handle);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            m_mappedPointer = nullptr;
        }
        if (m_handle != 0) {
            glDeleteBuffers(1, &m_handle);
        }
        if (m_syncObj) {
            glDeleteSync(m_syncObj);
            m_syncObj = nullptr;
        }
    }

    void* GetMappedPointer() const { 
        return m_mappedPointer; 
    }

private:
    uint32_t m_handle = 0;
    size_t m_size = 0;
    GLsync m_syncObj = nullptr;
    void* m_mappedPointer = nullptr; // Pointer to the mapped memory
};