#include <glad/glad.h>

struct SSBO {
public:
    uint32_t GetHandle() {
        return handle;
    }
    void PreAllocate(size_t size) {
        glCreateBuffers(1, &handle);
        glNamedBufferStorage(handle, (GLsizeiptr)size, NULL, GL_DYNAMIC_STORAGE_BIT);
        bufferSize = size;
    }
    void Update(size_t size, void* data) {
        if (size <= 0) {
            return;
        }
        if (handle == 0) {
            glCreateBuffers(1, &handle);
            glNamedBufferStorage(handle, (GLsizeiptr)size, NULL, GL_DYNAMIC_STORAGE_BIT);
            bufferSize = size;
        }
        if (bufferSize < size) {
            glDeleteBuffers(1, &handle);
            glCreateBuffers(1, &handle);
            glNamedBufferStorage(handle, (GLsizeiptr)size, NULL, GL_DYNAMIC_STORAGE_BIT);
            bufferSize = size;
        }
        glNamedBufferSubData(handle, 0, (GLsizeiptr)size, data);
    }
    void CleanUp() {
        if (handle != 0) {
            glDeleteBuffers(1, &handle);
        }
    }
private:
    uint32_t handle = 0;
    size_t bufferSize = 0;
};