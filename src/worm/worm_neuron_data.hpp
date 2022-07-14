#ifndef KOUEK_WORM_NEURON_DATA_H
#define KOUEK_WORM_NEURON_DATA_H

#include "worm_data.hpp"

namespace kouek {

class WormNeuronPositionData {
  private:
    class Parser {
      private:
        enum class State : uint8_t { Key, Val, ValX, ValY, ValZ };

      public:
        static void Parse(std::vector<glm::vec3> &dat,
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
            glm::vec3 *p = nullptr;
            string tmp;
            uint8_t idx = 0;
            State stat = State::Key;
            dat.clear();
            while (*itr) {
                switch (stat) {
                case State::Key:
                    if (*itr == ':')
                        stat = State::Val;
                    break;
                case State::Val:
                    if (*itr == '(') {
                        dat.emplace_back();
                        p = &dat.back();
                        beg = itr + 1;
                        stat = State::ValX;
                    }
                    break;
                    // Note: XYZ in neuron data is ZXY in worm
                case State::ValX:
                    if (*itr == ',') {
                        tmp.assign(beg, itr - beg);
                        p->z = stof(tmp);
                        beg = itr + 1;
                        stat = State::ValY;
                    }
                    break;
                case State::ValY:
                    if (*itr == ',') {
                        tmp.assign(beg, itr - beg);
                        p->x = stof(tmp);
                        beg = itr + 1;
                        stat = State::ValZ;
                    }
                    break;
                case State::ValZ:
                    if (*itr == ')') {
                        tmp.assign(beg, itr - beg);
                        p->y = stof(tmp);
                        beg = itr + 1;
                        stat = State::Key;
                    }
                    break;
                }
                ++itr;
            }
        }
    };

    GLuint VAO, VBO;
    std::string filePath;

    struct VertexDat {
        glm::vec3 pos;
        VertexDat(const glm::vec3 &cntrPos, const glm::vec3 &delta) {
            pos = cntrPos + delta;
        }
    };
    std::vector<std::vector<VertexDat>> verts;

  public:
    WormNeuronPositionData(std::string_view filePath,
                           std::shared_ptr<WormPositionData> wpd)
        : filePath(filePath) {
        std::vector<glm::vec3> dat;
        Parser::Parse(dat, this->filePath);

        /*glGenBuffers(1, &VBO);
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3,
                              (const void *)0);
        size_t nuroVertCnt = verts.front().size();
        size_t timeCnt = verts.size();
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexDat) * nuroVertCnt * timeCnt,
                     nullptr, GL_STATIC_DRAW);
        for (size_t t = 0; t < timeCnt; ++t)
            glBufferSubData(GL_ARRAY_BUFFER,
                            sizeof(VertexDat) * nuroVertCnt * t,
                            sizeof(VertexDat) * nuroVertCnt, verts[t].data());
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);*/
    }
    ~WormNeuronPositionData() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    inline const auto &GetVerts() const { return verts; }
    inline const auto GetVAO() { return VAO; }

  private:
    void transformToFitEachTimeStep() {}
};
} // namespace kouek

#endif // !KOUEK_WORM_NEURON_DATA_H
