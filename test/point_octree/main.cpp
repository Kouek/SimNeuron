#include <iostream>
#include <vector>
#include <random>

#include <util/point_octree.hpp>

using namespace kouek;

int main() {
    auto [min, max] = std::pair{glm::vec3{-1.f}, glm::vec3{1.f}};
    auto range = max - min;
    PointOctree<uint32_t> poctr(min, max);
    constexpr uint32_t POINT_NUM = 72;
    std::vector<glm::vec3> points;
    std::minstd_rand random;
    auto randomRng = random.max() - random.min();
    glm::vec3 pos;
    //for (uint32_t id = 0; id < POINT_NUM; ++id) {
    //    pos.x = (float)(random() - random.min()) / randomRng * range.x;
    //    pos.y = (float)(random() - random.min()) / randomRng * range.y;
    //    pos.z = (float)(random() - random.min()) / randomRng * range.z;
    //    pos += min;
    //    points.emplace_back(pos);
    //}

    //for (uint32_t id = 0; id < POINT_NUM; ++id)
    //    poctr.Insert(points[id], id);

    //// tree structure
    //std::cout.precision(4);
    //std::cout << poctr << std::endl;

    //// Query test
    //for (uint32_t id = 0; id < POINT_NUM; ++id) {
    //    auto [node, datIdx] = poctr.Query(points[id]);
    //    assert(node->dat[datIdx].second == id);
    //}

    // Query test 2
    PointOctree<uint32_t> poctr2(min, max, 2);
    constexpr uint32_t X_DIM = 10;
    constexpr uint32_t Y_DIM = 10;
    constexpr float SPACE = .1f;
    pos = {0, 0, 0};
    for (uint32_t x = 0; x < X_DIM; ++x) {
        pos.y = 0;
        for (uint32_t y = 0; y < Y_DIM; ++y) {
            poctr2.Insert(
                pos - glm::vec3{SPACE / 2 * X_DIM, SPACE / 2 * Y_DIM, 0},
                y * X_DIM + x);
            pos.y += SPACE;
        }
        pos.x += SPACE;
    }
    glm::vec3 p{0, 0, 1.f};
    std::array<glm::vec3, 4> drcs{
        glm::lookAt(p, glm::vec3{-1.f, -1.f, 0}, glm::vec3{0, 1.f, 0}) *
            glm::vec4{0, 0, -1.f, 0},
        glm::lookAt(p, glm::vec3{+1.f, -1.f, 0}, glm::vec3{0, 1.f, 0}) *
            glm::vec4{0, 0, -1.f, 0},
        glm::lookAt(p, glm::vec3{-1.f, +1.f, 0}, glm::vec3{0, 1.f, 0}) *
            glm::vec4{0, 0, -1.f, 0},
        glm::lookAt(p, glm::vec3{+1.f, +1.f, 0}, glm::vec3{0, 1.f, 0}) *
            glm::vec4{0, 0, -1.f, 0}};
    Frustum frustum(p, glm::vec3{0, 0, -1.f}, .01f, 10.f, drcs[0], drcs[2],
                    drcs[1], drcs[3]);
    auto selected = poctr2.Query(frustum);

    return 0;
}
