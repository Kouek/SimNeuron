#ifndef KOUEK_IMAGE_READ_H
#define KOUEK_IMAGE_READ_H

#include <tuple>
#include <string>

namespace kouek {
std::tuple<uint8_t *, int, int, int> ReadImageToRaw(const std::string &path);
void FreeRaw(uint8_t *dat);
} // namespace kouek

#endif // !KOUEK_IMAGE_READ_H
