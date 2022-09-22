#define STB_IMAGE_IMPLEMENTATION
#include "image_read.h"
#include <stb_image.h>

std::tuple<uint8_t *, int, int, int>
kouek::ReadImageToRaw(const std::string &path) {
    int width, height, nrChannels;
    uint8_t *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    return std::make_tuple(data, width, height, nrChannels);
}

void kouek::FreeRaw(uint8_t *dat) { stbi_image_free(dat); }
