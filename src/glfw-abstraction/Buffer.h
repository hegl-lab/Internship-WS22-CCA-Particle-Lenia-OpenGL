#ifndef GAME_OF_LIFE_BUFFER_H
#define GAME_OF_LIFE_BUFFER_H

#include <vector>
#include <glad/glad.h>

class Buffer {
public:
    Buffer();
    explicit Buffer(int size, int type);

    void init();

    void set_data(const std::vector<float> &data);
    std::vector<float> get_data() const;

    void bind(int index) const;

    void delete_buffer();

    GLuint id;
    int size;
    int type;
};


#endif //GAME_OF_LIFE_BUFFER_H
