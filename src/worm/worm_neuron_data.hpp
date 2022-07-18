#ifndef KOUEK_WORM_NEURON_DATA_H
#define KOUEK_WORM_NEURON_DATA_H

#include "worm_data.hpp"

#include <random>

#include <Eigen/Dense>

namespace kouek {

class WormNeuronPositionData {
  public:
    constexpr static size_t CURVE_VERT_COUNT = 1000;

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
    GLuint curveVAO, curveVBO;
    glm::vec3 maxPos{-std::numeric_limits<GLfloat>::infinity()},
        minPos{std::numeric_limits<GLfloat>::infinity()};
    std::string filePath;

    std::vector<glm::vec3> rawDat;
    struct VertexDat {
        glm::vec3 pos;
        VertexDat(const glm::vec3 &pos) : pos(pos) {}
        VertexDat(const glm::vec3 &cntrPos, const glm::vec3 &delta) {
            pos = cntrPos + delta;
        }
    };
    std::vector<std::vector<VertexDat>> verts;
    std::vector<VertexDat> curve;

    std::shared_ptr<WormPositionData> wpd;

  public:
    WormNeuronPositionData(std::string_view filePath,
                           std::shared_ptr<WormPositionData> wpd)
        : filePath(filePath), wpd(wpd) {
        Parser::Parse(rawDat, this->filePath);
        for (const auto &pos : rawDat)
            for (uint8_t xyz = 0; xyz < 3; ++xyz) {
                if (pos[xyz] < minPos[xyz])
                    minPos[xyz] = pos[xyz];
                if (pos[xyz] > maxPos[xyz])
                    maxPos[xyz] = pos[xyz];
            }

        size_t nuroVertCnt = rawDat.size();
        size_t timeCnt = wpd->GetVerts().size();
        glGenBuffers(1, &VBO);
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3,
                              (const void *)0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexDat) * nuroVertCnt * timeCnt,
                     nullptr, GL_STATIC_DRAW);

        glGenBuffers(1, &curveVBO);
        glGenVertexArrays(1, &curveVAO);
        glBindVertexArray(curveVAO);
        glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3,
                              (const void *)0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexDat) * CURVE_VERT_COUNT,
                     nullptr, GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        assignRawDatToVerts();
    }
    ~WormNeuronPositionData() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &curveVAO);
        glDeleteBuffers(1, &curveVBO);
    }
    void PolyCurveFitWith(uint8_t order, size_t searchTimes,
                          GLfloat inlierHalfWid) {
        auto coeffs = polyCurveFitWith(order, searchTimes, inlierHalfWid);
        auto &[wormMin, wormMax] = wpd->GetPosRange();
        glm::vec3 scale = glm::vec3{wormMax - wormMin, 0} / (maxPos - minPos);
        scale.z = 1.f;
        glm::vec3 offset0 = -.5f * (minPos + maxPos);
        glm::vec3 offset1 = +.5f * glm::vec3{wormMax + wormMin, 0};

        GLfloat x = minPos.x;
        GLfloat dx = (maxPos.x - minPos.x) / CURVE_VERT_COUNT;
        curve.clear();
        curve.reserve(CURVE_VERT_COUNT);
        for (size_t stepCnt = 0; stepCnt < CURVE_VERT_COUNT; ++stepCnt) {
            GLfloat curr = 1.f;
            GLfloat y = 0;
            for (uint8_t od = 0; od < order + 1; ++od) {
                y += coeffs[od] * curr;
                curr *= x;
            }
            curve.emplace_back(offset1 +
                               scale * (glm::vec3{x, y, 0} + offset0));
            x += dx;
        }
        glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        sizeof(VertexDat) * CURVE_VERT_COUNT, curve.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void RegisterWithWPD() {}
    inline const auto &GetVerts() const { return verts; }
    inline const auto GetVAO() const { return VAO; }
    inline const auto GetCurveVAO() const { return curveVAO; }

  private:
    inline Eigen::VectorXf polyCurveFitWith(uint8_t order, size_t searchTimes,
                                            GLfloat inlierHalfWid) {
        using namespace Eigen;

        uint8_t K = order + 1;
        MatrixXf A(K, K);
        VectorXf b(K);
        VectorXf x(K);
        for (size_t r = 0; r < K; ++r)
            A(r, 0) = 1.f;

        size_t maxInlierCnt = 0;
        std::vector<size_t> KSet;
        std::minstd_rand random;
        for (size_t t = 0; t < searchTimes; ++t) {
            KSet.clear();
            for (uint8_t k = 0; k < K; ++k) {
                size_t idx = random() % rawDat.size();
                while ([&]() -> bool {
                    for (const auto &val : KSet)
                        if (val == idx)
                            return true;
                    return false;
                }()) // de-duplicate
                    idx = random() % rawDat.size();
                KSet.emplace_back(idx);
            }
            auto coeffs = [&]() {
                for (size_t r = 0; r < K; ++r)
                    for (uint8_t c = 0; c < order; ++c)
                        A(r, c + 1) = A(r, c) * rawDat[KSet[r]].x;
                for (size_t r = 0; r < K; ++r)
                    b(r) = rawDat[KSet[r]].y;
                auto Qr = A.householderQr();
                return static_cast<VectorXf>(Qr.solve(b));
            }();
            {
                GLfloat x = minPos.x;
                GLfloat dx = (maxPos.x - minPos.x) / CURVE_VERT_COUNT;
                curve.clear();
                curve.reserve(CURVE_VERT_COUNT);
                for (size_t stepCnt = 0; stepCnt < CURVE_VERT_COUNT;
                     ++stepCnt) {
                    GLfloat curr = 1.f;
                    GLfloat y = 0;
                    for (uint8_t od = 0; od < order + 1; ++od) {
                        y += coeffs[od] * curr;
                        curr *= x;
                    }
                    curve.emplace_back(glm::vec3{x, y, 0});
                    x += dx;
                }
            }
            size_t inlierCnt = 0;
            for (const auto &pos : rawDat) {
                GLfloat dist = distBtwCurveAnd(pos);
                if (dist <= inlierHalfWid)
                    ++inlierCnt;
            }
            if (maxInlierCnt < inlierCnt) {
                x = coeffs;
                maxInlierCnt = inlierCnt;
            }
        }
        return x;
    }
    inline GLfloat distBtwCurveAnd(const glm::vec3 &pos) {
        GLfloat minDistSqr = std::numeric_limits<GLfloat>::max();
        for (size_t idx = 0; idx < CURVE_VERT_COUNT; ++idx) {
            auto d = curve[idx].pos - pos;
            GLfloat distSqr = glm::dot(d, d);
            if (distSqr < minDistSqr)
                minDistSqr = distSqr;
        }
        return sqrtf(minDistSqr);
    }
    inline void assignRawDatToVerts() {
        auto &[wormMin, wormMax] = wpd->GetPosRange();
        glm::vec3 scale = glm::vec3{wormMax - wormMin, 0} / (maxPos - minPos);
        scale.z = 1.f;
        glm::vec3 offset0 = -.5f * (minPos + maxPos);
        glm::vec3 offset1 = +.5f * glm::vec3{wormMax + wormMin, 0};
        size_t nuroVertCnt = rawDat.size();
        size_t timeCnt = wpd->GetVerts().size();

        verts.resize(timeCnt);
        verts.front().clear();
        for (const auto &pos : rawDat)
            verts.front().emplace_back(offset1 + scale * (pos + offset0));
        for (size_t t = 1; t < timeCnt; ++t)
            verts[t] = verts.front();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        for (size_t t = 0; t < timeCnt; ++t)
            glBufferSubData(GL_ARRAY_BUFFER,
                            sizeof(VertexDat) * nuroVertCnt * t,
                            sizeof(VertexDat) * nuroVertCnt, verts[t].data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};
} // namespace kouek

#endif // !KOUEK_WORM_NEURON_DATA_H
