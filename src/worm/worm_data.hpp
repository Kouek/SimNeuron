#ifndef KOUEK_WORM_DATA_H
#define KOUEK_WORM_DATA_H

#include <numeric>
#include <stdexcept>

#include <string>
#include <string_view>

#include <fstream>

#include <array>
#include <map>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace kouek {
class WormPositionData {
  private:
    class Parser {
      private:
        /// <summary>
        /// Data in worm position file is managed in format:
        /// worm_position: [
        ///   [(x00,y00), (x01,y01)], [(...), (...)], ...
        /// ]
        /// worm_position: [
        ///   [(x10,y10), (x11,y11)], [(...), (...)], ...
        /// ]
        /// Thus, states below plus an idx variable
        /// indicating the first () or the second ()
        /// are adequate to parse the data.
        /// </summary>
        enum class State : uint8_t { Key, Val0, Val1, Val2X, Val2Y };

      public:
        static void
        Parse(std::vector<std::vector<std::array<glm::vec3, 2>>> &dat,
              const std::string &filePath) {
            using namespace std;

            ifstream in(filePath.data(), ios::ate | ifstream::binary);
            if (!in.is_open())
                throw runtime_error("Cannot open file: " + filePath);

            auto fileSize = in.tellg();
            in.seekg(ios::beg);

            char *buffer = new char[static_cast<size_t>(fileSize) + 1];
            in.read(buffer, fileSize);
            buffer[static_cast<size_t>(fileSize)] = '\0';

            in.close();

            char *itr = buffer;
            char *beg = nullptr;
            std::vector<std::array<glm::vec3, 2>> *p2s = nullptr;
            std::array<glm::vec3, 2> *p2 = nullptr;
            string tmp;
            uint8_t idx = 0;
            State stat = State::Key;
            dat.clear();
            while (*itr) {
                switch (stat) {
                case State::Key:
                    if (*itr == '[') {
                        dat.emplace_back();
                        p2s = &(dat.back());
                        stat = State::Val0;
                    }
                    break;
                case State::Val0:
                    switch (*itr) {
                    case '[':
                        stat = State::Val1;
                        p2s->emplace_back();
                        p2 = &(p2s->back());
                        (*p2)[0].z = (*p2)[1].z = 0;
                        break;
                    case ']':
                        stat = State::Key;
                        break;
                    }
                    break;
                case State::Val1:
                    switch (*itr) {
                    case '(':
                        stat = State::Val2X;
                        beg = itr + 1;
                        break;
                    case ']':
                        stat = State::Val0;
                        break;
                    }
                    break;
                case State::Val2X:
                    if (*itr == ',') {
                        tmp.assign(beg, itr - beg);
                        (*p2)[idx].x = stof(tmp);
                        stat = State::Val2Y;
                        beg = itr + 1;
                    }
                    break;
                case State::Val2Y:
                    if (*itr == ')') {
                        tmp.assign(beg, itr - beg);
                        (*p2)[idx].y = stof(tmp);
                        stat = State::Val1;
                        idx = (idx + 1) % 2;
                    }
                    break;
                }
                ++itr;
            }
            if (stat != State::Key || dat.size() == 0)
                throw runtime_error("File is not valid.");
            size_t segSz = dat.front().size();
            if (segSz % 2 != 0)
                throw runtime_error("Segments should be given in pair to "
                                    "compute central path of worm.");
            for (const auto &seg : dat)
                if (seg.size() != segSz)
                    throw runtime_error("File doesn't offer same segment size "
                                        "among different time steps.");

            delete[] buffer;
        }
    };

    GLuint VAO, VBO;
    glm::vec2 maxPos{-std::numeric_limits<float>::infinity()},
        minPos{std::numeric_limits<float>::infinity()};
    std::string filePath;

    std::vector<std::tuple<glm::vec2, glm::vec2>> posRanges;

    struct VertexDat {
        glm::vec3 cntrPos;
        glm::vec3 delta;
        VertexDat(const glm::vec3 &cntrPos, const glm::vec3 &delta)
            : cntrPos(cntrPos), delta(delta) {}
    };
    std::vector<std::vector<VertexDat>> verts;
    std::map<GLfloat, size_t> cntrPath;

  public:
    WormPositionData(const std::string_view filePath) : filePath(filePath) {
        // extract data from contour file
        {
            std::vector<std::vector<std::array<glm::vec3, 2>>> dat;
            Parser::Parse(dat, this->filePath);
            size_t segSz = dat.front().size();

            posRanges.resize(
                dat.size(),
                std::forward_as_tuple(
                    glm::vec3{+std::numeric_limits<float>::infinity()},
                    glm::vec3{-std::numeric_limits<float>::infinity()}));
            for (size_t timeStep = 0; timeStep < dat.size(); ++timeStep)
                for (const auto &p2 : dat[timeStep])
                    for (uint8_t idx = 0; idx < 2; ++idx) {
                        auto &[min, max] = posRanges[timeStep];
                        for (uint8_t xy = 0; xy < 2; ++xy) {
                            if (p2[idx][xy] < minPos[xy])
                                minPos[xy] = p2[idx][xy];
                            if (p2[idx][xy] > maxPos[xy])
                                maxPos[xy] = p2[idx][xy];

                            if (p2[idx][xy] < min[xy])
                                min[xy] = p2[idx][xy];
                            if (p2[idx][xy] > max[xy])
                                max[xy] = p2[idx][xy];
                        }
                    }

            glm::vec3 cntrPos, delta;
            for (const auto &segs : dat) {
                verts.emplace_back();
                auto &vertsT = verts.back();
                for (size_t pairIdx = 0; pairIdx < segSz; pairIdx += 2) {
                    delta = .5f * (segs[pairIdx][0] - segs[pairIdx + 1][0]);
                    cntrPos = .5f * (segs[pairIdx][0] + segs[pairIdx + 1][0]);
                    vertsT.emplace_back(cntrPos, delta);
                    // the 2nd point will be the 1st point in next
                    // pair, no need to append it

                    cntrPath.emplace(cntrPos.x, vertsT.size() - 1);
                }
            }
            // set the z of positions in t(0) and t(n-1) to -1.0 and 1,0,
            // inform shader that they are in t(0) and t(n-1)
            for (auto &vertsT : verts) {
                vertsT.front().cntrPos.z = -1.f;
                vertsT.back().cntrPos.z = 1.f;
            }
        }

        glGenBuffers(1, &VBO);
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6,
                              (const void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6,
                              (const void *)sizeof(glm::vec3));
        size_t wormVertCnt = verts.front().size();
        size_t timeCnt = verts.size();
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexDat) * wormVertCnt * timeCnt,
                     nullptr, GL_STATIC_DRAW);
        for (size_t t = 0; t < timeCnt; ++t)
            glBufferSubData(GL_ARRAY_BUFFER,
                            sizeof(VertexDat) * wormVertCnt * t,
                            sizeof(VertexDat) * wormVertCnt, verts[t].data());
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    ~WormPositionData() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    inline const auto &GetVerts() const { return verts; }
    inline const auto &GetCntrPath() const { return cntrPath; }
    inline const auto GetVAO() { return VAO; }
    inline const auto GetPosRange() const {
        return std::make_tuple(minPos, maxPos);
    }
    inline const auto GetPosRangeOf(size_t timeStep) const {
        return posRanges[timeStep];
    }
};
} // namespace kouek

#endif // !KOUEK_WORM_DATA_H
