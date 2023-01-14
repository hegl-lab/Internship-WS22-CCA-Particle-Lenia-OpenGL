#include <cstring>
#include "Buffer.h"

Buffer::Buffer() {}

Buffer::Buffer(int size, int type) : size(size), type(type) {}

void Buffer::init() {
    glGenBuffers(1, &id);
    glBindBuffer(type, id);
    glBufferData(type, sizeof(float) * size, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(type, 0);
}

void Buffer::set_data(const std::vector<float> &data) {
    glBindBuffer(type, id);
    GLvoid* p = glMapBuffer(type, GL_WRITE_ONLY);
    memcpy(p, data.data(), sizeof(float) * size);
    glUnmapBuffer(type);
}

std::vector<float> Buffer::get_data() const {
    std::vector<float> data(size);
    glBindBuffer(type, id);
    glGetBufferSubData(type, 0, size * sizeof(float), data.data());
    glUnmapBuffer(type);
    return data;
}

void Buffer::bind(int index) const {
    glBindBufferBase(type, index, id);
}

void Buffer::delete_buffer() {
    glDeleteBuffers(1, &id);
    id = -1;
}



