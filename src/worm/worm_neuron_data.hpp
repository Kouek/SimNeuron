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
    glm::vec3 maxPos{-std::numeric_limits<GLfloat>::infinity()},
        minPos{std::numeric_limits<GLfloat>::infinity()};
    std::string filePath;

    std::vector<glm::vec3> rawDat;
    std::vector<std::vector<glm::vec3>> verts;
    std::vector<glm::vec3> curve;
    std::unordered_set<size_t> inliers;
    std::array<std::unordered_set<size_t>, 3> cmpInliers;

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
    void SelectAndAppendComponentInliers(WormPositionData::Component component,
                                         const Frustum &frustm) {
        auto cmpIdx = static_cast<uint8_t>(component);
        for (size_t rdIdx = 0; rdIdx < rawDat.size(); ++rdIdx)
            if (frustm.IsIntersetcedWith(rawDat[rdIdx])) {
                bool exist = false;
                for (uint8_t idx = 0; idx < 3; ++idx)
                    if (idx != cmpIdx && cmpInliers[idx].count(rdIdx) != 0) {
                        exist = true;
                        break;
                    }
                if (!exist) {
                    cmpInliers[cmpIdx].emplace(rdIdx);
                    inliers.emplace(rdIdx);
                }
            }
        uploadCmpInliers();
    }
    inline void UnselectComponent(WormPositionData::Component component) {
        auto cmpIdx = static_cast<uint8_t>(component);
        for (const auto val : cmpInliers[cmpIdx])
            inliers.erase(val);
        cmpInliers[cmpIdx].clear();
        uploadCmpInliers();
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
    inline const auto
    GetComponentInliersVertCnt(WormPositionData::Component component) const {
        return cmpInliers[static_cast<uint8_t>(component)].size();
    }
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
    inline void uploadCmpInliers() {
        std::vector<GLuint> plainInliers;
        plainInliers.reserve(inliers.size());
        for (uint8_t cmpIdx = 0; cmpIdx < 3; ++cmpIdx) {
            for (auto id : cmpInliers[cmpIdx])
                plainInliers.emplace_back(id);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inliersEBO);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                        sizeof(GLuint) * plainInliers.size(),
                        plainInliers.data());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
        static constexpr auto VC_DIST_RATIO_TO_BOT = .25f;

        if (wpd->GetVerts().size() == 0 || wpd->GetVerts().front().size() == 0)
            return;
        size_t timeCnt = wpd->GetVerts().size();
        size_t wpdVertCnt = wpd->GetVerts().front().size();

        std::vector<GLfloat> curveLens;
        curveLens.reserve(curve.size());
        curveLens.emplace_back(0);
        for (size_t idx = 1; idx < curve.size(); ++idx)
            curveLens.emplace_back(curveLens.back() +
                                   glm::distance(curve[idx - 1], curve[idx]));

        float VCInliersMaxDistToCurv = std::numeric_limits<float>::min();
        std::array<std::array<size_t, 2>, 3> curveCmpRange{
            std::array<size_t, 2>{std::numeric_limits<size_t>::max(),
                                  std::numeric_limits<size_t>::min()},
            std::array<size_t, 2>{std::numeric_limits<size_t>::max(),
                                  std::numeric_limits<size_t>::min()},
            std::array<size_t, 2>{std::numeric_limits<size_t>::max(),
                                  std::numeric_limits<size_t>::min()},
        };
        std::vector<size_t> rawDatToCurve;
        rawDatToCurve.reserve(rawDat.size());
        std::vector<size_t> outliers;
        outliers.reserve(rawDat.size() - inliers.size());
        for (size_t rdIdx = 0; rdIdx < rawDat.size(); ++rdIdx) {
            auto [curveIdx, dist] = [&]() {
                float dist = std::numeric_limits<float>::max();
                size_t idx = 0;
                for (size_t cvIdx = 0; cvIdx < curve.size(); ++cvIdx) {
                    auto currDist = glm::distance(curve[cvIdx], rawDat[rdIdx]);
                    if (currDist < dist) {
                        idx = cvIdx;
                        dist = currDist;
                    }
                }
                return std::make_tuple(idx, dist);
            }();
            rawDatToCurve.emplace_back(curveIdx);
            auto cmpIdx = [&](float dist) {
                for (uint8_t cmpIdx = 0; cmpIdx < 3; ++cmpIdx)
                    if (cmpInliers[cmpIdx].count(rdIdx) != 0) {
                        if (cmpIdx == 1 && VCInliersMaxDistToCurv < dist)
                            VCInliersMaxDistToCurv = dist; // VC
                        return cmpIdx;
                    }
                return (uint8_t)4;
            }(dist);
            if (cmpIdx == 4) {
                // outliers
                outliers.emplace_back(rdIdx);
                continue;
            }
            if (curveCmpRange[cmpIdx][0] > curveIdx)
                curveCmpRange[cmpIdx][0] = curveIdx;
            if (curveCmpRange[cmpIdx][1] < curveIdx)
                curveCmpRange[cmpIdx][1] = curveIdx;
        }

        std::array wpdCmpStartEnds = {
            wpd->GetComponentStartEnd(WormPositionData::Component::Head),
            wpd->GetComponentStartEnd(WormPositionData::Component::VentralCord),
            wpd->GetComponentStartEnd(WormPositionData::Component::Tail)};
        auto maxRefLineSz =
            std::max({wpdCmpStartEnds[0][1] - wpdCmpStartEnds[0][0],
                      wpdCmpStartEnds[1][1] - wpdCmpStartEnds[1][0],
                      wpdCmpStartEnds[2][1] - wpdCmpStartEnds[2][0]});
        std::vector<glm::vec3> refLine;
        refLine.reserve(maxRefLineSz);
        std::vector<float> refLineLens;
        refLineLens.reserve(maxRefLineSz);

        std::vector<std::tuple<size_t, float, glm::vec3>> rawDatToWPD(
            rawDat.size());

        // compute VC reference line first fro dltScale
        auto &wpdVertsT0 = wpd->GetVerts().front();
        float dltScale = std::numeric_limits<float>::max();
        for (auto wpdIdx = wpdCmpStartEnds[1][0];
             wpdIdx < wpdCmpStartEnds[1][1]; ++wpdIdx) {
            auto distToBot =
                VC_DIST_RATIO_TO_BOT * glm::length(wpdVertsT0[wpdIdx].delta);
            if (dltScale > distToBot)
                dltScale = distToBot;
            refLine.emplace_back(wpdVertsT0[wpdIdx].cntrPos +
                                 (1.f - 2 * VC_DIST_RATIO_TO_BOT) *
                                     wpdVertsT0[wpdIdx].delta);
        }
        dltScale = dltScale / VCInliersMaxDistToCurv;
        refLineLens.emplace_back(0);
        for (size_t rfIdx = 1; rfIdx < refLine.size(); ++rfIdx)
            refLineLens.emplace_back(
                refLineLens.back() +
                glm::distance(refLine[rfIdx - 1], refLine[rfIdx]));

        auto computeRefIdx = [&](size_t cvIdx, uint8_t cmpIdx) {
            auto len =
                (curveLens[cvIdx] - curveLens[curveCmpRange[cmpIdx][0]]) /
                (curveLens[curveCmpRange[cmpIdx][1]] -
                 curveLens[curveCmpRange[cmpIdx][0]]);
            len *= refLineLens.back();
            size_t left = 0, right = refLineLens.size() - 1;
            while (left < right) {
                auto mid = (left + right) / 2;
                auto midVal = refLineLens[mid];
                if ((mid == refLineLens.size() - 1 && midVal <= len) ||
                    (midVal <= len && refLineLens[mid + 1] > len)) {
                    left = mid;
                    break;
                } else if (midVal < len)
                    left = mid + 1;
                else
                    right = mid - 1;
            }
            auto refSegLen = left == refLineLens.size() - 1
                                 ? refLineLens[left] - refLineLens[left - 1]
                                 : refLineLens[left + 1] - refLineLens[left];
            return std::make_pair(left, (len - refLineLens[left]) / refSegLen);
        };
        auto computeRotMat = [](const std::vector<glm::vec3> &curve0,
                                const std::vector<glm::vec3> &curve1,
                                size_t idx0, size_t idx1) {
            auto tgnLn0 = idx0 == 0 ? curve0[1] - curve0[0]
                          : idx0 == curve0.size() - 1
                              ? curve0[idx0] - curve0[idx0 - 1]
                              : curve0[idx0 + 1] - curve0[idx0];
            tgnLn0.z = 0;
            auto tgnLn1 = idx1 == 0 ? curve1[1] - curve1[0]
                          : idx1 == curve1.size() - 1
                              ? curve1[idx1] - curve1[idx1 - 1]
                              : curve1[idx1 + 1] - curve1[idx1];
            tgnLn1.z = 0;
            auto cos = glm::dot(tgnLn0, tgnLn1) / glm::length(tgnLn0) /
                       glm::length(tgnLn1);
            auto sin = sqrtf(1 - cos * cos);
            return (tgnLn0.x * tgnLn1.y - tgnLn1.x * tgnLn0.y) < 0
                       ? glm::mat3{cos, -sin, 0, +sin, cos, 0, 0, 0, 1.f}
                       : glm::mat3{cos, +sin, 0, -sin, cos, 0, 0, 0, 1.f};
        };
        auto &vertsT0 = verts.front();
        auto warpRawDatToT0 = [&](size_t rdIdx, uint8_t cmpIdx) {
            auto &[rfIdx, refSegOffsRatio, dlt] = rawDatToWPD[rdIdx];
            auto cvIdx = rawDatToCurve[rdIdx];
            std::tie(rfIdx, refSegOffsRatio) = computeRefIdx(cvIdx, cmpIdx);
            dlt = computeRotMat(curve, refLine, cvIdx, rfIdx) *
                  (rawDat[rdIdx] - curve[cvIdx]);

            auto cntrPos =
                rfIdx == refLine.size() - 1
                    ? refLine[rfIdx] + refSegOffsRatio *
                                           (refLine[rfIdx] - refLine[rfIdx - 1])
                    : refLine[rfIdx] + refSegOffsRatio * (refLine[rfIdx + 1] -
                                                          refLine[rfIdx]);
            vertsT0[rdIdx] = cntrPos + dltScale * dlt;
        };
        for (auto rdIdx : cmpInliers[1])
            warpRawDatToT0(rdIdx, 1);
        for (auto rdIdx : outliers)
            warpRawDatToT0(rdIdx, 1);

        // compute head reference line
        refLine.clear();
        for (auto wpdIdx = wpdCmpStartEnds[0][0];
             wpdIdx < wpdCmpStartEnds[0][1]; ++wpdIdx)
            refLine.emplace_back(wpdVertsT0[wpdIdx].cntrPos);
        refLineLens.clear();
        refLineLens.emplace_back(0);
        for (size_t rfIdx = 1; rfIdx < refLine.size(); ++rfIdx)
            refLineLens.emplace_back(
                refLineLens.back() +
                glm::distance(refLine[rfIdx - 1], refLine[rfIdx]));
        for (auto rdIdx : cmpInliers[0])
            warpRawDatToT0(rdIdx, 0);

        // compute tail reference line
        refLine.clear();
        for (auto wpdIdx = wpdCmpStartEnds[2][0];
             wpdIdx < wpdCmpStartEnds[2][1]; ++wpdIdx)
            refLine.emplace_back(wpdVertsT0[wpdIdx].cntrPos);
        refLineLens.clear();
        refLineLens.emplace_back(0);
        for (size_t rfIdx = 1; rfIdx < refLine.size(); ++rfIdx)
            refLineLens.emplace_back(
                refLineLens.back() +
                glm::distance(refLine[rfIdx - 1], refLine[rfIdx]));
        for (auto rdIdx : cmpInliers[2])
            warpRawDatToT0(rdIdx, 2);

        auto computeRotMatWPD = [](decltype(wpdVertsT0) curve0,
                                   decltype(wpdVertsT0) curve1, size_t idx0,
                                   size_t idx1) {
            auto tgnLn0 = idx0 == 0 ? curve0[1].cntrPos - curve0[0].cntrPos
                          : idx0 == curve0.size() - 1
                              ? curve0[idx0].cntrPos - curve0[idx0 - 1].cntrPos
                              : curve0[idx0 + 1].cntrPos - curve0[idx0].cntrPos;
            tgnLn0.z = 0;
            auto tgnLn1 = idx1 == 0 ? curve1[1].cntrPos - curve1[0].cntrPos
                          : idx1 == curve1.size() - 1
                              ? curve1[idx1].cntrPos - curve1[idx1 - 1].cntrPos
                              : curve1[idx1 + 1].cntrPos - curve1[idx1].cntrPos;
            tgnLn1.z = 0;
            auto cos = glm::dot(tgnLn0, tgnLn1) / glm::length(tgnLn0) /
                       glm::length(tgnLn1);
            cos = glm::clamp(cos, 0.f, 1.f);
            auto sin = sqrtf(1 - cos * cos);
            return (tgnLn0.x * tgnLn1.y - tgnLn1.x * tgnLn0.y) < 0
                       ? glm::mat3{cos, -sin, 0, +sin, cos, 0, 0, 0, 1.f}
                       : glm::mat3{cos, +sin, 0, -sin, cos, 0, 0, 0, 1.f};
        };
        for (size_t timeStep = 1; timeStep < timeCnt; ++timeStep) {
            auto& wpdVertsT = wpd->GetVerts()[timeStep];
            auto& vertsT = verts[timeStep];

            auto warpRawDatToT = [&](size_t rdIdx, uint8_t cmpIdx) {
                auto [rfIdx, refSegOffsRatio, dlt] = rawDatToWPD[rdIdx];
                auto wpdIdx = wpdCmpStartEnds[cmpIdx][0] + rfIdx;
                dlt = computeRotMatWPD(wpdVertsT0, wpdVertsT, wpdIdx, wpdIdx) *
                      dlt;
                auto cntrPos =
                    rfIdx == refLine.size() - 1
                        ? refLine[rfIdx] +
                              refSegOffsRatio *
                                  (refLine[rfIdx] - refLine[rfIdx - 1])
                        : refLine[rfIdx] +
                              refSegOffsRatio *
                                  (refLine[rfIdx + 1] - refLine[rfIdx]);
                vertsT[rdIdx] = cntrPos + dltScale * dlt;
            };

            // VC
            refLine.clear();
            dltScale = std::numeric_limits<float>::max();
            for (auto wpdIdx = wpdCmpStartEnds[1][0];
                 wpdIdx < wpdCmpStartEnds[1][1]; ++wpdIdx) {
                auto distToBot =
                    VC_DIST_RATIO_TO_BOT * glm::length(wpdVertsT[wpdIdx].delta);
                if (dltScale > distToBot)
                    dltScale = distToBot;
                refLine.emplace_back(wpdVertsT[wpdIdx].cntrPos +
                                     (1.f - 2 * VC_DIST_RATIO_TO_BOT) *
                                         wpdVertsT[wpdIdx].delta);
            }
            dltScale = dltScale / VCInliersMaxDistToCurv;
            for (auto rdIdx : cmpInliers[1])
                warpRawDatToT(rdIdx, 1);
            for (auto rdIdx : outliers)
                warpRawDatToT(rdIdx, 1);

            // head
            refLine.clear();
            for (auto wpdIdx = wpdCmpStartEnds[0][0];
                 wpdIdx < wpdCmpStartEnds[0][1]; ++wpdIdx)
                refLine.emplace_back(wpdVertsT[wpdIdx].cntrPos);
            for (auto rdIdx : cmpInliers[0])
                warpRawDatToT(rdIdx, 0);

            // tail
            refLine.clear();
            for (auto wpdIdx = wpdCmpStartEnds[2][0];
                 wpdIdx < wpdCmpStartEnds[2][1]; ++wpdIdx)
                refLine.emplace_back(wpdVertsT[wpdIdx].cntrPos);
            for (auto rdIdx : cmpInliers[2])
                warpRawDatToT(rdIdx, 2);
        }
    }
};
} // namespace kouek

#endif // !KOUEK_WORM_NEURON_DATA_H
