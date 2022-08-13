#ifndef KOUEK_WORM_NEURON_DATA_H
#define KOUEK_WORM_NEURON_DATA_H

#include "worm_data.hpp"

#include <unordered_set>

#include <util/math.h>

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
    GLuint slctVAO, slctEBO;
    GLuint slctFrameVAO, slctFrameVBO, slctFrameEBO;
    GLuint curveVAO, curveVBO;
    bool inlierFrameDatChanged = true;
    glm::vec3 maxPos{-std::numeric_limits<GLfloat>::infinity()},
        minPos{std::numeric_limits<GLfloat>::infinity()};
    glm::vec3 offset, scale; // map vertices to [(-1,-1),(1,1)]
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
    std::unordered_set<size_t> inliers;

    std::shared_ptr<WormPositionData> wpd;
    //std::unique_ptr<PointOctree<size_t>> octree;

  public:
    WormNeuronPositionData(std::string_view filePath,
                           std::shared_ptr<WormPositionData> wpd)
        : filePath(filePath), wpd(wpd) {
        Parser::Parse(rawDat, this->filePath);
        for (const auto &pos : rawDat) {
            for (uint8_t xyz = 0; xyz < 3; ++xyz) {
                if (pos[xyz] < minPos[xyz])
                    minPos[xyz] = pos[xyz];
                if (pos[xyz] > maxPos[xyz])
                    maxPos[xyz] = pos[xyz];
            }
        }
        {
            scale = 2.f / (maxPos - minPos);
            scale.z = 1.f;
            offset = -.5f * (minPos + maxPos);
        }

        size_t nuroVertCnt = rawDat.size();
        size_t timeCnt = wpd->GetVerts().size();
        glGenBuffers(1, &VBO);
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexDat),
                              (const void *)0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexDat) * nuroVertCnt * timeCnt,
                     nullptr, GL_STATIC_DRAW);

        glGenBuffers(1, &slctEBO);
        glGenVertexArrays(1, &slctVAO);
        glBindVertexArray(slctVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexDat),
                              (const void *)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, slctEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * nuroVertCnt,
                     nullptr, GL_STATIC_DRAW);
       

        glGenBuffers(1, &slctFrameVBO);
        glGenBuffers(1, &slctFrameEBO);
        glGenVertexArrays(1, &slctFrameVAO);
        glBindVertexArray(slctFrameVAO);
        glBindBuffer(GL_ARRAY_BUFFER, slctFrameVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexDat),
                              (const void *)0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexDat) * 4, nullptr,
                     GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, slctFrameEBO);
        {
            std::array<GLubyte, 5> idx{0, 1, 3, 2, 0};
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * idx.size(),
                         idx.data(), GL_STATIC_DRAW);
        }

        glGenBuffers(1, &curveVBO);
        glGenVertexArrays(1, &curveVAO);
        glBindVertexArray(curveVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // !slctFrameEBO
        glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexDat),
                              (const void *)0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexDat) * CURVE_VERT_COUNT,
                     nullptr, GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        assignRawDatToVerts();
        /*octree = std::make_unique<PointOctree<size_t>>(
            scale * (minPos + offset), scale * (maxPos + offset), 4);
        if (verts.size() != 0 && verts.front().size() != 0)
            for (size_t idx = 0; idx < rawDat.size(); ++idx)
                octree->Insert(verts.front()[idx].pos, idx);*/
    }
    ~WormNeuronPositionData() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &slctVAO);
        glDeleteBuffers(1, &slctEBO);
        glDeleteVertexArrays(1, &slctFrameVAO);
        glDeleteBuffers(1, &slctFrameVBO);
        glDeleteBuffers(1, &slctFrameEBO);
        glDeleteVertexArrays(1, &curveVAO);
        glDeleteBuffers(1, &curveVBO);
    }
    void SetSelectedFrame(const std::array<glm::vec3, 4> &verts) {
        glBindBuffer(GL_ARRAY_BUFFER, slctFrameVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VertexDat) * 4,
                        verts.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void PolyCurveFitWith(uint8_t order) {
        Eigen::VectorXf coeffs = polyCurveFitWith(order);

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
            curve.emplace_back(scale * (glm::vec3{x, y, 0} + offset));
            x += dx;
        }
        glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        sizeof(VertexDat) * CURVE_VERT_COUNT, curve.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        assignRawDatToVerts();
    }
    void RegisterWithWPD() {

    }
    inline void SelectAndAppendInliers(const Frustum& frustm) {
        inlierFrameDatChanged = true;
        /*auto slcted = octree->Query(frustm);
        for (auto node : slcted)
            for (uint8_t datIdx = 0; datIdx < node->datNum; ++datIdx)
                inliers.emplace(node->dat[datIdx].second);*/
        for (size_t vIdx = 0; vIdx < verts.front().size(); ++vIdx)
            if (frustm.IsIntersetcedWith(verts.front()[vIdx].pos))
                inliers.emplace(vIdx);
        std::vector<GLuint> plainInliers;
        plainInliers.reserve(inliers.size());
        for (auto id : inliers)
            plainInliers.emplace_back(id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, slctEBO);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                        sizeof(GLuint) * plainInliers.size(),
                        plainInliers.data());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    inline void UnselectInliers() { inliers.clear();
    }
    inline const auto &GetVerts() const { return verts; }
    inline const auto GetVAO() const { return VAO; }
    inline const auto GetSelectedVAO() const { return slctVAO; }
    inline const auto GetSelectedVertCnt() const { return inliers.size(); }
    inline const auto GetSelctingFrameVAO() const { return slctFrameVAO; }
    inline const auto GetCurveVAO() const { return curveVAO; }

  private:
    void assignRawDatToVerts() {
        size_t nuroVertCnt = rawDat.size();
        size_t timeCnt = wpd->GetVerts().size();

        verts.resize(timeCnt);
        verts.front().clear();
        for (const auto &pos : rawDat)
            verts.front().emplace_back(scale * (pos + offset));
        for (size_t t = 1; t < timeCnt; ++t)
            verts[t] = verts.front();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        for (size_t t = 0; t < timeCnt; ++t)
            glBufferSubData(GL_ARRAY_BUFFER,
                            sizeof(VertexDat) * nuroVertCnt * t,
                            sizeof(VertexDat) * nuroVertCnt, verts[t].data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    Eigen::VectorXf polyCurveFitWith(uint8_t order) {
        using namespace Eigen;
        MatrixXf A(inliers.size(), order + 1);
        VectorXf b(inliers.size());
        size_t row = 0;
        for (auto idx : inliers) {
            GLfloat curr = 1.f;
            for (size_t col = 0; col <= order; ++col) {
                A(row, col) = curr;
                curr *= rawDat[idx].x;
            }
            b(row) = rawDat[idx].y;
            ++row;
        }
        auto QR = A.householderQr();
        return QR.solve(b);
    }
};
} // namespace kouek

#endif // !KOUEK_WORM_NEURON_DATA_H
