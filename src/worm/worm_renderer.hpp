#ifndef KOUEK_WORM_RENDERER_H
#define KOUEK_WORM_RENDERER_H

#include <memory>
#include <vector>

#include <cmake_in.h>

#include <util/shader.h>

#include "worm_data.hpp"
#include "worm_neuron_data.hpp"

#include "image_read.h"

#define GL_CHECK                                                               \
    {                                                                          \
        GLenum gl_err;                                                         \
        if ((gl_err = glGetError()) != GL_NO_ERROR) {                          \
            std::cout << "OpenGL error: " << gl_err                            \
                      << " caused before on line: " << __LINE__                \
                      << " of file: " << __FILE__ << std::endl;                \
        }                                                                      \
    }

namespace kouek {

class WormRenderer {
  public:
    enum class SceneMode : uint8_t { Full, Focus };
    enum class RenderTarget : uint8_t {
        Worm,
        NeuronReg,
        WormReg,
        WormAndNeuron
    };
    struct LightParam {
        float ambientStrength = .1f;
        glm::vec3 lightColor{1.f};
        glm::vec3 lightPos{0, 0, .5f};
    };

  private:
    bool cameraChanged = true, divNumChanged = true, lightChanged = true;
    bool sceneModeChanged = true, timeStepChanged = true,
         renderTargetChanged = true;
    bool drawWireFrame = false;
    uint8_t divNum;
    GLuint backgroundVAO, backgroundVBO, backgroundEBO, backgroundTex;
    GLuint frameVAO, frameVBO, frameEBO;
    size_t wormVertCnt, nuroVertCnt, curveVertCnt;
    size_t timeStep = 0;
    GLfloat nuroHfWid = .01f, frontFaceOpacity = 1.f;
    GLfloat backgroundZ, minScaleThroughT, avgScaleThroughT;
    glm::mat4 view, proj;
    LightParam lightParam;
    SceneMode sceneMode = SceneMode::Full;
    RenderTarget renderTarget = RenderTarget::Worm;
    std::unique_ptr<Shader> wormShader, nuroShader, bkgrndShader, normalShader;
    std::shared_ptr<WormPositionData> wpd;
    std::shared_ptr<WormNeuronPositionData> wnpd;

  public:
    WormRenderer() {
        glGenBuffers(1, &backgroundVBO);
        glGenBuffers(1, &backgroundEBO);
        glGenVertexArrays(1, &backgroundVAO);
        glBindVertexArray(backgroundVAO);
        glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4),
                              (const void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4),
                              (const void *)sizeof(glm::vec2));
        {
            std::array verts{glm::vec4{-1.f, -1.f, +0.f, +0.f},
                             glm::vec4{+1.f, -1.f, +1.f, +0.f},
                             glm::vec4{-1.f, +1.f, +0.f, +1.f},
                             glm::vec4{+1.f, +1.f, +1.f, +1.f}};
            for (auto &v : verts)
                for (uint8_t xy = 0; xy < 2; ++xy)
                    v[xy] *= 1.5f;
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * verts.size(),
                         verts.data(), GL_STATIC_DRAW);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backgroundEBO);
        {
            std::array<GLubyte, 6> indices{0, 1, 2, 2, 1, 3};
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         sizeof(GLubyte) * indices.size(), indices.data(),
                         GL_STATIC_DRAW);
        }
        glGenTextures(1, &backgroundTex);
        glBindTexture(GL_TEXTURE_2D, backgroundTex);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            auto [dat, width, height, nrChannels] = ReadImageToRaw(
                std::string(PROJECT_SOURCE_DIR) + "/data/ChessBoard.jpg");
            assert(dat != nullptr);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, dat);
            glGenerateMipmap(GL_TEXTURE_2D);
            FreeRaw(dat);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenBuffers(1, &frameVBO);
        glGenBuffers(1, &frameEBO);
        glGenVertexArrays(1, &frameVAO);
        glBindVertexArray(frameVAO);
        glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                              (const void *)0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 4, nullptr,
                     GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, frameEBO);
        {
            std::array<GLubyte, 5> indices{0, 1, 3, 2, 0};
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         sizeof(GLubyte) * indices.size(), indices.data(),
                         GL_STATIC_DRAW);
        }

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        wormShader =
            std::make_unique<Shader>((std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/worm3D.vs")
                                         .c_str(),
                                     (std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/worm3D.fs")
                                         .c_str(),
                                     (std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/worm3D.gs")
                                         .c_str());
        nuroShader =
            std::make_unique<Shader>((std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/neuron3D.vs")
                                         .c_str(),
                                     (std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/neuron3D.fs")
                                         .c_str(),
                                     (std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/neuron3D.gs")
                                         .c_str());
        bkgrndShader =
            std::make_unique<Shader>((std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/background3D.vs")
                                         .c_str(),
                                     (std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/background3D.fs")
                                         .c_str());
        normalShader =
            std::make_unique<Shader>((std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/normal3D.vs")
                                         .c_str(),
                                     (std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/normal3D.fs")
                                         .c_str());
        normalShader->use();
        normalShader->setMat4("M", glm::identity<glm::mat4>());
        normalShader->setMat4("VP", glm::identity<glm::mat4>());
    }
    ~WormRenderer() {
        glDeleteVertexArrays(1, &backgroundVAO);
        glDeleteBuffers(1, &backgroundVBO);
        glDeleteBuffers(1, &backgroundEBO);
        glDeleteVertexArrays(1, &frameVAO);
        glDeleteBuffers(1, &frameVBO);
        glDeleteBuffers(1, &frameEBO);
    }
    void SetTimeStep(size_t timeStep) {
        this->timeStep = timeStep;
        timeStepChanged = true;
    }
    void SetDrawWireFrame(bool drawWireFrame) {
        this->drawWireFrame = drawWireFrame;
    }
    void SetSceneMode(SceneMode sceneMode) {
        this->sceneMode = sceneMode;
        sceneModeChanged = true;
    }
    void SetRenderTarget(RenderTarget renderTarget) {
        this->renderTarget = renderTarget;
        renderTargetChanged = true;
    }
    inline auto GetRenderTarget() const { return renderTarget; }
    void SetWormPositionDat(std::shared_ptr<WormPositionData> dat) {
        wpd = dat;
        wnpd.reset();
        if (!dat)
            return;
        wormVertCnt = wpd->GetVerts().front().size();
        {
            backgroundZ = 0;
            for (const auto &vertsT : wpd->GetVerts())
                for (const auto &vert : vertsT) {
                    auto len = glm::length(vert.delta);
                    if (len > backgroundZ)
                        backgroundZ = len;
                }
            backgroundZ *= -2.f;
        }
        minScaleThroughT = 1.f;
        avgScaleThroughT = 0;
        auto timeCnt = wpd->GetVerts().size();
        for (size_t timeStep = 0; timeStep < timeCnt; ++timeStep) {
            auto [min, max] = wpd->GetPosRangeOf(timeStep);
            glm::vec2 delta = max - min;
            auto scale = 2.f / std::max({delta.x, delta.y});
            if (minScaleThroughT > scale)
                minScaleThroughT = scale;
            avgScaleThroughT += scale / (float)timeCnt;
        }
    }
    void SetWormNeuronPositionDat(std::shared_ptr<WormNeuronPositionData> dat) {
        wnpd = dat;
        if (!dat)
            return;
        nuroVertCnt = wnpd->GetVerts().front().size();
    }
    void SetDivisionNum(uint8_t divNum) {
        this->divNum = divNum;
        divNumChanged = true;
    }
    void SetCamera(const glm::mat4 &view, const glm::mat4 &proj) {
        this->view = view;
        this->proj = proj;
        cameraChanged = true;
    }
    void SetLightParam(const LightParam &param) {
        lightParam = param;
        lightChanged = true;
    }
    const auto &GetLightParam() const { return lightParam; }
    void Render() {
        static constexpr glm::vec3 WORM_BACK_COLOR{.4f, .6f, .1f};
        static constexpr glm::vec3 WORM_FRONT_COLOR{.6f, .6f, .4f};
        static constexpr glm::vec3 NURO_COLOR{.4f, .8f, 1.f};
        static constexpr glm::vec3 SLCT_COLOR{1.f, .1f, .3f};
        static constexpr std::array<glm::vec3, 3> CMP_COLORS{
            glm::vec3{1.f, 0, 0}, glm::vec3{1.f, 0, 1.f}, glm::vec3{0, 1.f, 0}};

        if (divNumChanged) {
            std::vector<GLfloat> vals(divNum);

            wormShader->use();
            glUniform1ui(glGetUniformLocation(wormShader->ID, "divNum"),
                         (GLuint)divNum);
            for (uint8_t idx = 0; idx < divNum; ++idx)
                vals[idx] = cosf(glm::pi<float>() * 2.f * idx / divNum);
            glUniform1fv(glGetUniformLocation(wormShader->ID, "coss"), divNum,
                         vals.data());
            for (uint8_t idx = 0; idx < divNum; ++idx)
                vals[idx] = sinf(glm::pi<float>() * 2.f * idx / divNum);
            glUniform1fv(glGetUniformLocation(wormShader->ID, "sins"), divNum,
                         vals.data());

            auto nuroDivNum = 12; // limited by geometry shader
            vals.resize(nuroDivNum);
            nuroShader->use();
            glUniform1ui(glGetUniformLocation(nuroShader->ID, "divNum"),
                         (GLuint)nuroDivNum);
            for (uint8_t idx = 0; idx < nuroDivNum; ++idx)
                vals[idx] = cosf(glm::pi<float>() * 2.f * idx / nuroDivNum);
            glUniform1fv(glGetUniformLocation(nuroShader->ID, "coss"),
                         nuroDivNum, vals.data());
            for (uint8_t idx = 0; idx < nuroDivNum; ++idx)
                vals[idx] = sinf(glm::pi<float>() * 2.f * idx / nuroDivNum);
            glUniform1fv(glGetUniformLocation(nuroShader->ID, "sins"),
                         nuroDivNum, vals.data());

            divNumChanged = false;
        }

        if (sceneModeChanged || renderTargetChanged || timeStepChanged) {
            sceneModeChanged = renderTargetChanged = timeStepChanged = false;

            if (sceneMode == SceneMode::Full) {
                switch (renderTarget) {
                case RenderTarget::Worm: {
                    if (!wpd)
                        break;
                    auto [min, max] = wpd->GetPosRange();
                    glm::vec3 offset{-.5f * (max + min), 0};
                    glm::vec2 delta = max - min;
                    glm::vec3 scale{2.f / std::max({delta.x, delta.y})};
                    auto M = glm::scale(glm::identity<glm::mat4>(), scale) *
                             glm::translate(glm::identity<glm::mat4>(), offset);
                    wormShader->use();
                    wormShader->setMat4("M", M);
                    bkgrndShader->use();
                    bkgrndShader->setFloat("z", scale.x * backgroundZ);
                    break;
                }
                case RenderTarget::NeuronReg: {
                    if (!wnpd)
                        break;
                    auto [min, max] = wnpd->GetPosRange();
                    glm::vec3 offset{-.5f * (max + min)};
                    offset.z = 0;
                    glm::vec2 delta = max - min;
                    glm::vec3 scale{2.f / std::max({delta.x, delta.y})};
                    auto M = glm::scale(glm::identity<glm::mat4>(), scale) *
                             glm::translate(glm::identity<glm::mat4>(), offset);
                    nuroShader->use();
                    nuroShader->setMat4("M", M);
                    break;
                }
                case RenderTarget::WormReg: {
                    if (!wpd)
                        break;
                    auto [min, max] = wpd->GetPosRangeOf(0);
                    glm::vec3 offset{-.5f * (max + min), 0};
                    glm::vec2 delta = max - min;
                    glm::vec3 scale{2.f / std::max({delta.x, delta.y})};
                    auto M = glm::scale(glm::identity<glm::mat4>(), scale) *
                             glm::translate(glm::identity<glm::mat4>(), offset);
                    wormShader->use();
                    wormShader->setMat4("M", M);
                    nuroShader->use();
                    nuroShader->setMat4("M", M);
                    bkgrndShader->use();
                    bkgrndShader->setFloat("z", minScaleThroughT * backgroundZ);
                    break;
                }
                case RenderTarget::WormAndNeuron: {
                    if (!wpd)
                        break;
                    auto [min, max] = wpd->GetPosRange();
                    glm::vec3 offset{-.5f * (max + min), 0};
                    glm::vec2 delta = max - min;
                    glm::vec3 scale{2.f / std::max({delta.x, delta.y})};
                    auto M = glm::scale(glm::identity<glm::mat4>(), scale) *
                             glm::translate(glm::identity<glm::mat4>(), offset);
                    wormShader->use();
                    wormShader->setMat4("M", M);
                    nuroShader->use();
                    nuroShader->setMat4("M", M);
                    bkgrndShader->use();
                    bkgrndShader->setFloat("z", scale.x * backgroundZ);
                    break;
                }
                }
            } else if (sceneMode == SceneMode::Focus &&
                       renderTarget != RenderTarget::NeuronReg && wpd) {
                auto [min, max] = wpd->GetPosRangeOf(timeStep);
                glm::vec3 offset{-.5f * (max + min), 0};
                auto M = glm::scale(glm::identity<glm::mat4>(),
                                    glm::vec3{avgScaleThroughT}) *
                         glm::translate(glm::identity<glm::mat4>(), offset);
                wormShader->use();
                wormShader->setMat4("M", M);
                nuroShader->use();
                nuroShader->setMat4("M", M);
                bkgrndShader->use();
                bkgrndShader->setFloat("z", minScaleThroughT * backgroundZ);
            }
        }

        if (cameraChanged) {
            auto VP = proj * view;
            wormShader->use();
            wormShader->setMat4("VP", VP);
            nuroShader->use();
            nuroShader->setMat4("VP", VP);
            bkgrndShader->use();
            bkgrndShader->setMat4("VP", VP);

            cameraChanged = false;
        }
        if (lightChanged) {
            wormShader->use();
            wormShader->setFloat("ambientStrength", lightParam.ambientStrength);
            wormShader->setVec3("lightColor", lightParam.lightColor);
            wormShader->setVec3("lightPos", lightParam.lightPos);

            nuroShader->use();
            nuroShader->setFloat(
                "ambientStrength",
                glm::clamp(lightParam.ambientStrength + .2f, 0.f, 1.f));
            nuroShader->setVec3("lightColor", lightParam.lightColor);
            nuroShader->setVec3("lightPos", lightParam.lightPos);

            lightChanged = false;
        }

        if (drawWireFrame)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        switch (renderTarget) {
        case RenderTarget::Worm:
            if (!wpd)
                break;
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            bkgrndShader->use();
            glBindTexture(GL_TEXTURE_2D, backgroundTex);
            glBindVertexArray(backgroundVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            glCullFace(GL_FRONT);

            wormShader->use();
            wormShader->setVec3("color", WORM_BACK_COLOR);
            wormShader->setBool("reverseNormal", true);
            glBindVertexArray(wpd->GetVAO());
            glDrawArrays(GL_LINE_STRIP, timeStep * wormVertCnt, wormVertCnt);

            glCullFace(GL_BACK);
            if (frontFaceOpacity < 1.f) {
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
                glBlendColor(0, 0, 0, frontFaceOpacity);
            }

            wormShader->setVec3("color", WORM_FRONT_COLOR);
            wormShader->setBool("reverseNormal", false);
            glBindVertexArray(wpd->GetVAO());
            glDrawArrays(GL_LINE_STRIP, timeStep * wormVertCnt, wormVertCnt);

            glBindVertexArray(0);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glDisable(GL_BLEND);
            break;
        case RenderTarget::NeuronReg: {
            if (!wnpd)
                break;

            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            nuroShader->use();
            nuroShader->setVec3("color", NURO_COLOR);
            nuroShader->setFloat("halfWid", nuroHfWid);
            glBindVertexArray(wnpd->GetVAO());
            glDrawArrays(GL_POINTS, timeStep * nuroVertCnt, nuroVertCnt);

            std::array<size_t, 3> inliersCnt{
                wnpd->GetComponentInliersVertCnt(
                    WormPositionData::Component::Head),
                wnpd->GetComponentInliersVertCnt(
                    WormPositionData::Component::VentralCord),
                wnpd->GetComponentInliersVertCnt(
                    WormPositionData::Component::Tail)};
            size_t offset = 0;
            nuroShader->setFloat("halfWid", nuroHfWid * 1.1f);
            glBindVertexArray(wnpd->GetInliersVAO());
            for (uint8_t cmpIdx = 0; cmpIdx < 3; ++cmpIdx) {
                nuroShader->setVec3("color", CMP_COLORS[cmpIdx]);
                glDrawElements(GL_POINTS, inliersCnt[cmpIdx], GL_UNSIGNED_INT,
                               (const void *)(sizeof(GLuint) * offset));
                offset += inliersCnt[cmpIdx];
            }

            glDisable(GL_DEPTH_TEST);

            nuroShader->setVec3("color", SLCT_COLOR);
            nuroShader->setFloat("halfWid", nuroHfWid * 1.2f);
            glBindVertexArray(wnpd->GetCurveVAO());
            glDrawArrays(GL_POINTS, 0, wnpd->GetCurveVertCnt());

            normalShader->use();
            normalShader->setVec3("color", SLCT_COLOR);
            glBindVertexArray(frameVAO);
            glDrawElements(GL_LINE_STRIP, 5, GL_UNSIGNED_BYTE, 0);

            glBindVertexArray(0);
            glDisable(GL_CULL_FACE);
            break;
        }
        case RenderTarget::WormReg:
            if (!wpd)
                break;
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            bkgrndShader->use();
            glBindTexture(GL_TEXTURE_2D, backgroundTex);
            glBindVertexArray(backgroundVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            wormShader->use();
            wormShader->setVec3("color", WORM_BACK_COLOR);
            wormShader->setBool("reverseNormal", false);
            glBindVertexArray(wpd->GetVAO());
            glDrawArrays(GL_LINE_STRIP, timeStep * wormVertCnt, wormVertCnt);

            glDisable(GL_DEPTH_TEST);

            for (uint8_t cmpIdx = 0; cmpIdx < 3; ++cmpIdx) {
                auto cmp = static_cast<WormPositionData::Component>(cmpIdx);
                auto vertCnt = wpd->GetComponentVertCnt(cmp);
                wormShader->setVec3("color", CMP_COLORS[cmpIdx]);
                glBindVertexArray(wpd->GetComponentVAO(cmp));
                glDrawElements(GL_LINE_STRIP, vertCnt, GL_UNSIGNED_INT, 0);
            }

            glDisable(GL_CULL_FACE);
            glBindVertexArray(0);
            break;
        case RenderTarget::WormAndNeuron:
            if (!wpd || !wnpd)
                break;
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            bkgrndShader->use();
            glBindTexture(GL_TEXTURE_2D, backgroundTex);
            glBindVertexArray(backgroundVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            nuroShader->use();
            nuroShader->setVec3("color", NURO_COLOR);
            nuroShader->setFloat("halfWid", nuroHfWid);
            glBindVertexArray(wnpd->GetVAO());
            glDrawArrays(GL_POINTS, timeStep * nuroVertCnt, nuroVertCnt);

            glCullFace(GL_FRONT);

            wormShader->use();
            wormShader->setVec3("color", WORM_BACK_COLOR);
            wormShader->setBool("reverseNormal", true);
            glBindVertexArray(wpd->GetVAO());
            glDrawArrays(GL_LINE_STRIP, timeStep * wormVertCnt, wormVertCnt);

            glCullFace(GL_BACK);
            if (frontFaceOpacity < 1.f) {
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
                glBlendColor(0, 0, 0, frontFaceOpacity);
            }

            wormShader->setVec3("color", WORM_FRONT_COLOR);
            wormShader->setBool("reverseNormal", false);
            glBindVertexArray(wpd->GetVAO());
            glDrawArrays(GL_LINE_STRIP, timeStep * wormVertCnt, wormVertCnt);

            glBindVertexArray(0);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glDisable(GL_BLEND);
            break;
        }
        if (drawWireFrame)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    void SetFrameVerts(const std::array<glm::vec2, 2> &pos2) {
        std::array<glm::vec3, 4> pos4;
        for (uint8_t idx = 0; idx < 4; ++idx)
            pos4[idx] = glm::vec3{pos2[(idx & 0x1) ? 1 : 0].x,
                                  pos2[(idx & 0x2) ? 1 : 0].y, 0};
        glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * 4, pos4.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void SetNeuronHalfWidth(float hfWid) { nuroHfWid = hfWid; }
    void SetFrontFaceOpacity(float opacity) { frontFaceOpacity = opacity; }
};

} // namespace kouek

#endif // !KOUEK_WORM_RENDERER_H
