#ifndef KOUEK_WORM_NEURON_DATA_H
#define KOUEK_WORM_NEURON_DATA_H

#include "worm_data.hpp"

#include <unordered_set>

#include <util/math.h>

#include <Eigen/Dense>

namespace kouek {

class WormNeuronPositionData {
  public:
    static constexpr uint8_t CURVE_SAMPLE_MULT = 10;

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
    GLuint inliersVAO, inliersEBO;
    GLuint curveVAO, curveVBO;
    size_t wormVertCnt;
    bool inlierFrameDatChanged = true;
    glm::vec3 maxPos{-std::numeric_limits<GLfloat>::infinity()},
        minPos{std::numeric_limits<GLfloat>::infinity()};
    glm::vec3 inliersMaxPos{-std::numeric_limits<GLfloat>::infinity()},
        inliersMinPos{std::numeric_limits<GLfloat>::infinity()};
    std::string filePath;

    std::vector<glm::vec3> rawDat;
    std::vector<std::vector<glm::vec3>> verts;
    std::vector<glm::vec3> curve;
    std::unordered_set<size_t> inliers;

    std::shared_ptr<WormPositionData> wpd;

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

        if (rawDat.empty() || wpd->GetVerts().empty() ||
            wpd->GetVerts().front().empty())
            return;
        wormVertCnt = wpd->GetVerts().front().size();
        size_t nuroVertCnt = rawDat.size();
        size_t timeCnt = wpd->GetVerts().size();

        glGenBuffers(1, &VBO);
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                              (const void *)0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * nuroVertCnt * timeCnt,
                     nullptr, GL_STATIC_DRAW);

        glGenBuffers(1, &inliersEBO);
        glGenVertexArrays(1, &inliersVAO);
        glBindVertexArray(inliersVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                              (const void *)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inliersEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * nuroVertCnt,
                     nullptr, GL_STATIC_DRAW);

        glGenBuffers(1, &curveVBO);
        glGenVertexArrays(1, &curveVAO);
        glBindVertexArray(curveVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                              (const void *)0);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(glm::vec3) * wormVertCnt * CURVE_SAMPLE_MULT,
                     nullptr, GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        assignRawDatToVerts();
    }
    ~WormNeuronPositionData() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &inliersVAO);
        glDeleteBuffers(1, &inliersEBO);
        glDeleteVertexArrays(1, &curveVAO);
        glDeleteBuffers(1, &curveVBO);
    }
    void PolyCurveFitWith(uint8_t order) {
        if (rawDat.empty() || wpd->GetVerts().empty() ||
            wpd->GetVerts().front().empty())
            return;

        Eigen::VectorXf coeffs = polyCurveFitWith(order);
        auto curveZ = [&]() {
            GLfloat z = 0;
            for (auto idx : inliers)
                z += rawDat[idx].z;
            z /= inliers.size();
            return z;
        }();

        size_t curveCnt = wormVertCnt * CURVE_SAMPLE_MULT;
        GLfloat x = minPos.x;
        GLfloat dx = (maxPos.x - minPos.x) / curveCnt;
        curve.clear();
        curve.reserve(curveCnt);
        for (size_t stepCnt = 0; stepCnt < curveCnt; ++stepCnt) {
            GLfloat curr = 1.f;
            GLfloat y = 0;
            for (uint8_t od = 0; od < order + 1; ++od) {
                y += coeffs[od] * curr;
                curr *= x;
            }
            curve.emplace_back(glm::vec3{x, y, curveZ});
            x += dx;
        }
        glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * curveCnt,
                        curve.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        assignRawDatToVerts();
    }
    void SelectAndAppendInliers(const Frustum &frustm) {
        inlierFrameDatChanged = true;

        for (size_t vIdx = 0; vIdx < verts.front().size(); ++vIdx)
            if (frustm.IsIntersetcedWith(verts.front()[vIdx]))
                inliers.emplace(vIdx);
        std::vector<GLuint> plainInliers;
        plainInliers.reserve(inliers.size());
        for (auto id : inliers) {
            plainInliers.emplace_back(id);
            auto &pos = rawDat[id];
            for (uint8_t xyz = 0; xyz < 3; ++xyz) {
                if (pos[xyz] < inliersMinPos[xyz])
                    inliersMinPos[xyz] = pos[xyz];
                if (pos[xyz] > inliersMaxPos[xyz])
                    inliersMaxPos[xyz] = pos[xyz];
            }
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inliersEBO);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                        sizeof(GLuint) * plainInliers.size(),
                        plainInliers.data());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    inline void UnselectInliers() {
        inliers.clear();
        inliersMaxPos = glm::vec3{-std::numeric_limits<GLfloat>::max()};
        inliersMinPos = glm::vec3{+std::numeric_limits<GLfloat>::max()};
    }
    inline void ClearCurve() { curve.clear(); }
    void RegisterWithWPD() {
        if (curve.empty() || rawDat.empty() || wpd->GetVerts().empty() ||
            wpd->GetVerts().front().empty())
            return;

        registerWithWPD();

        size_t nuroVertCnt = rawDat.size();
        size_t timeCnt = wpd->GetVerts().size();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        for (size_t t = 0; t < timeCnt; ++t)
            glBufferSubData(GL_ARRAY_BUFFER,
                            sizeof(glm::vec3) * nuroVertCnt * t,
                            sizeof(glm::vec3) * nuroVertCnt, verts[t].data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    inline const auto &GetVerts() const { return verts; }
    inline const auto GetVAO() const { return VAO; }
    inline const auto GetInliersVAO() const { return inliersVAO; }
    inline const auto GetInliersVertCnt() const { return inliers.size(); }
    inline const auto GetCurveVAO() const { return curveVAO; }
    inline const auto GetCurveVertCnt() const { return curve.size(); }
    inline const auto GetPosRange() const {
        return std::make_tuple(minPos, maxPos);
    }

  private:
    void assignRawDatToVerts() {
        size_t nuroVertCnt = rawDat.size();
        size_t timeCnt = wpd->GetVerts().size();

        verts.resize(timeCnt);
        verts.front().clear();
        for (const auto &pos : rawDat)
            verts.front().emplace_back(pos);
        for (size_t t = 1; t < timeCnt; ++t)
            verts[t] = verts.front();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        for (size_t t = 0; t < timeCnt; ++t)
            glBufferSubData(GL_ARRAY_BUFFER,
                            sizeof(glm::vec3) * nuroVertCnt * t,
                            sizeof(glm::vec3) * nuroVertCnt, verts[t].data());
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
    void registerWithWPD() {
        static constexpr auto VC_DIST_RATIO_TO_BOT = .1f;

        if (wpd->GetVerts().size() == 0 || wpd->GetVerts().front().size() == 0)
            return;
        size_t timeCnt = wpd->GetVerts().size();
        size_t wormVertCnt = wpd->GetVerts().front().size();

        std::vector<GLfloat> curveLens;
        curveLens.reserve(curve.size());
        curveLens.emplace_back(0);
        for (size_t idx = 1; idx < curve.size(); ++idx)
            curveLens.emplace_back(curveLens[idx - 1] +
                                   glm::distance(curve[idx - 1], curve[idx]));

        float inliersAvgDistToCurve = 0;
        std::vector<std::tuple<GLfloat, size_t>> lenAndCurveIndices;
        lenAndCurveIndices.reserve(rawDat.size());
        for (size_t vIdx = 0; vIdx < rawDat.size(); ++vIdx) {
            auto [curveIdx, dist] = [&]() {
                GLfloat dist = std::numeric_limits<GLfloat>::max();
                size_t idx = 0;
                for (size_t cIdx = 0; cIdx < curve.size(); ++cIdx) {
                    auto currDist = glm::distance(curve[cIdx], rawDat[vIdx]);
                    if (currDist < dist) {
                        idx = cIdx;
                        dist = currDist;
                    }
                }
                return std::make_pair(idx, dist);
            }();
            lenAndCurveIndices.emplace_back(curveLens[curveIdx], curveIdx);
            if (inliers.count(vIdx) != 0)
                inliersAvgDistToCurve += dist;
        }
        inliersAvgDistToCurve /= inliers.size();

        std::vector<glm::vec3> VC;
        VC.reserve(wormVertCnt);
        std::vector<GLfloat> VCLens;
        VCLens.reserve(wormVertCnt);
        std::map<GLfloat, GLuint> VCLenToIndices;
        auto startEnds = [&]() {
            return std::array{
                wpd->GetComponentStartEnd(WormPositionData::Component::Head),
                wpd->GetComponentStartEnd(
                    WormPositionData::Component::VentralCord),
                wpd->GetComponentStartEnd(WormPositionData::Component::Tail)};
        }();
        for (size_t timeStep = 0; timeStep < timeCnt; ++timeStep) {
            auto wpdVertsT = wpd->GetVerts()[timeStep];

            VC.clear();
            // Head component, move along central line
            for (GLuint idx = startEnds[0][0]; idx < startEnds[0][1]; ++idx)
                VC.emplace_back(wpdVertsT[idx].cntrPos);
            // transistion between Head and VC component,
            // use linear interpolation
            {
                auto startPos = VC.back();
                auto dlt = wpdVertsT[startEnds[1][0]].cntrPos - startPos;
                float dltIdx = startEnds[1][0] - startEnds[0][1];
                for (GLuint idx = startEnds[0][1]; idx < startEnds[1][0]; ++idx)
                    VC.emplace_back(startPos +
                                    (idx - startEnds[0][1]) / dltIdx * dlt);
            }
            // VC component, move along bottom
            float VCAvgDistToBottom = 0;
            for (GLuint idx = startEnds[1][0]; idx < startEnds[1][1]; ++idx) {
                VC.emplace_back(wpdVertsT[idx].cntrPos +
                                (1.f - 2 * VC_DIST_RATIO_TO_BOT) *
                                    wpdVertsT[idx].delta);
                VCAvgDistToBottom +=
                    VC_DIST_RATIO_TO_BOT * glm::length(wpdVertsT[idx].delta);
            }
            VCAvgDistToBottom /= (startEnds[1][1] - startEnds[1][0]);
            // transistion between VC and Tail component,
            // use linear interpolation
            {
                auto startPos = VC.back();
                auto dlt = wpdVertsT[startEnds[2][0]].cntrPos - startPos;
                float dltIdx = startEnds[2][0] - startEnds[1][1];
                for (GLuint idx = startEnds[1][1]; idx < startEnds[2][0]; ++idx)
                    VC.emplace_back(startPos +
                                    (idx - startEnds[1][1]) / dltIdx * dlt);
            }
            // Tail component, move along central line
            for (GLuint idx = startEnds[2][0]; idx < startEnds[2][1]; ++idx)
                VC.emplace_back(wpdVertsT[idx].cntrPos);

            VCLens.clear();
            VCLens.emplace_back(0);
            for (GLuint idx = 1; idx < VC.size(); ++idx)
                VCLens.emplace_back(VCLens[idx - 1] +
                                    glm::distance(VC[idx - 1], VC[idx]));
            VCLenToIndices.clear();
            for (GLuint idx = 0; idx < VC.size(); ++idx)
                VCLenToIndices[VCLens[idx]] = idx;

            auto lenScale = VCLens.back() / curveLens.back();
            auto dltScale = VCAvgDistToBottom / inliersAvgDistToCurve;
            for (size_t vIdx = 0; vIdx < rawDat.size(); ++vIdx) {
                auto [len, curveIdx] = lenAndCurveIndices[vIdx];
                auto scaledLen = lenScale * len;
                auto right = VCLenToIndices.lower_bound(scaledLen);
                if (right == VCLenToIndices.end())
                    --right;
                auto cntrPos = [&]() {
                    auto lenDltRat =
                        (scaledLen - right->first) /
                        (right->second == 0 ? VCLens[1] - VCLens[0]
                                            : VCLens[right->second] -
                                                  VCLens[right->second - 1]);
                    auto lenDlt =
                        right->second == 0
                            ? VC[1] - VC[0]
                            : VC[right->second] - VC[right->second - 1];
                    return VC[right->second] + lenDltRat * lenDlt;
                }();
                auto rotMat = [&](size_t curveIdx) {
                    auto curveTgnLnDrc =
                        curveIdx == 0 ? curve[1] - curve[0]
                        : curveIdx == curve.size() - 1
                            ? curve[curveIdx] - curve[curveIdx - 1]
                            : curve[curveIdx + 1] - curve[curveIdx - 1];
                    auto VCTgnLnDrc =
                        right->second == 0 ? VC[1] - VC[0]
                        : right->second == VC.size() - 1
                            ? VC[right->second] - VC[right->second - 1]
                            : VC[right->second + 1] - VC[right->second - 1];
                    auto cos = glm::dot(curveTgnLnDrc, VCTgnLnDrc) /
                               glm::length(curveTgnLnDrc) /
                               glm::length(VCTgnLnDrc);
                    auto sin = sqrtf(1 - cos * cos);
                    return (curveTgnLnDrc.x * VCTgnLnDrc.y -
                            VCTgnLnDrc.x * curveTgnLnDrc.y) > 0
                               ? glm::mat3{cos, -sin, 0, +sin, cos,
                                           0,   0,    0, 1.f}
                               : glm::mat3{cos, +sin, 0, -sin, cos,
                                           0,   0,    0, 1.f};
                }(curveIdx);
                verts[timeStep][vIdx] =
                    cntrPos +
                    dltScale * rotMat * (rawDat[vIdx] - curve[curveIdx]);
            }
        }
    }
};
} // namespace kouek

#endif // !KOUEK_WORM_NEURON_DATA_H
