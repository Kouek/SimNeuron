#ifndef KOUEK_WORM_RENDERER_H
#define KOUEK_WORM_RENDERER_H

#include <memory>
#include <vector>

#include <cmake_in.h>

#include <util/shader.h>

#include "worm_data.hpp"
#include "worm_neuron_data.hpp"

namespace kouek {

class WormRenderer {
  public:
    enum class SceneMode : uint8_t { Full, Focus };

  protected:
    bool drawWireFrame = false;
    bool sceneModeChanged = true, timeStepChanged = true;
    SceneMode sceneMode = SceneMode::Full;
    size_t timeStep = 0;
    std::shared_ptr<WormPositionData> wpd;
    std::shared_ptr<WormNeuronPositionData> wnpd;

  public:
    virtual void SetWormPositionDat(std::shared_ptr<WormPositionData> dat) {
        wpd = dat;
        wnpd.reset();
    }
    virtual void
    SetWormNeuronPositionDat(std::shared_ptr<WormNeuronPositionData> dat) {
        wnpd = dat;
    }
    virtual void SetTimeStep(size_t timeStep) {
        this->timeStep = timeStep;
        timeStepChanged = true;
    }
    virtual void SetDrawWireFrame(bool drawWireFrame) {
        this->drawWireFrame = drawWireFrame;
    }
    virtual void SetSceneMode(SceneMode sceneMode) {
        this->sceneMode = sceneMode;
        sceneModeChanged = true;
        timeStepChanged = true;
    }
    virtual void Render() = 0;
};

class WormRenderer2D : public WormRenderer {
  private:
    size_t wormVertCnt;
    std::unique_ptr<Shader> wormShader;

  public:
    WormRenderer2D() {
        wormShader =
            std::make_unique<Shader>((std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/worm2D.vs")
                                         .c_str(),
                                     (std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/worm2D.fs")
                                         .c_str(),
                                     (std::string(kouek::PROJECT_SOURCE_DIR) +
                                      "/src/worm/shader/worm2D.gs")
                                         .c_str());
    }
    void SetWormPositionDat(std::shared_ptr<WormPositionData> dat) override {
        WormRenderer::SetWormPositionDat(dat);
        if (!dat)
            return;
        wormVertCnt = wpd->GetVerts().front().size();
    }
    void SetWormNeuronPositionDat(std::shared_ptr<WormNeuronPositionData> dat) {
        WormRenderer::SetWormNeuronPositionDat(dat);
        if (!dat)
            return;
    }
    void Render() override {
        wormShader->use();
        if (sceneModeChanged) {
            if (sceneMode == SceneMode::Full && wpd) {
                auto [min, max] = wpd->GetPosRange();
                glm::vec2 offset = -.5f * (max + min);
                glm::vec2 scale = max - min;
                scale = glm::vec2{2.f / std::max({scale.x, scale.y})};
                wormShader->setVec2("offset", offset);
                wormShader->setVec2("scale", scale);
            }
            sceneModeChanged = false;
        }
        if (sceneMode == SceneMode::Focus && timeStepChanged && wpd) {
            auto [min, max] = wpd->GetPosRangeOf(timeStep);
            glm::vec2 offset = -.5f * (max + min);
            glm::vec2 scale = max - min;
            scale = glm::vec2{2.f / std::max({scale.x, scale.y})};
            wormShader->setVec2("offset", offset);
            wormShader->setVec2("scale", scale);
        }
        if (timeStepChanged)
            timeStepChanged = false;
        if (wpd) {
            wormShader->setVec4("color", glm::vec4{1.f});
            glBindVertexArray(wpd->GetVAO());
            glDrawArrays(GL_LINE_STRIP, timeStep * wormVertCnt, wormVertCnt);
            glBindVertexArray(0);
        }
        // if (wnpd) {
        //    shader->setVec4("color", glm::vec4{1.f, .5f, .1f, 1.f});
        //    glBindVertexArray(wnpd->GetVAO2D());
        //    glDrawArrays(GL_POINTS, 0, wnpd->GetSize());
        //    glBindVertexArray(0);
        //}
    }
};

class WormRenderer3D : public WormRenderer {
  private:
    bool cameraChanged = true, divNumChanged = true;
    uint8_t divNum;
    size_t wormVertCnt, nuroVertCnt;
    glm::mat4 view, proj;
    std::unique_ptr<Shader> wormShader, nuroShader;

  public:
    WormRenderer3D() {
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
                                         .c_str());
    }
    void SetWormPositionDat(std::shared_ptr<WormPositionData> dat) override {
        WormRenderer::SetWormPositionDat(dat);
        if (!dat)
            return;
        wormVertCnt = wpd->GetVerts().front().size();
    }
    void SetWormNeuronPositionDat(std::shared_ptr<WormNeuronPositionData> dat) {
        WormRenderer::SetWormNeuronPositionDat(dat);
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
    void Render() override {
        if (divNumChanged) {
            wormShader->use();
            glUniform1ui(glGetUniformLocation(wormShader->ID, "divNum"),
                         (GLuint)divNum);

            std::vector<GLfloat> vals(divNum);
            for (uint8_t idx = 0; idx < divNum; ++idx)
                vals[idx] = cosf(glm::pi<float>() * 2.f * idx / divNum);
            glUniform1fv(glGetUniformLocation(wormShader->ID, "coss"), divNum,
                         vals.data());
            for (uint8_t idx = 0; idx < divNum; ++idx)
                vals[idx] = sinf(glm::pi<float>() * 2.f * idx / divNum);
            glUniform1fv(glGetUniformLocation(wormShader->ID, "sins"), divNum,
                         vals.data());

            divNumChanged = false;
        }

        if (sceneModeChanged) {
            if (sceneMode == SceneMode::Full && wpd) {
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
            }
            sceneModeChanged = false;
        }
        if (sceneMode == SceneMode::Focus && timeStepChanged && wpd) {
            wormShader->use();
            auto [min, max] = wpd->GetPosRangeOf(timeStep);
            glm::vec3 offset{-.5f * (max + min), 0};
            glm::vec2 delta = max - min;
            glm::vec3 scale{2.f / std::max({delta.x, delta.y})};
            auto M = glm::scale(glm::identity<glm::mat4>(), scale) *
                     glm::translate(glm::identity<glm::mat4>(), offset);
            wormShader->use();
            wormShader->setMat4("M", M);
            nuroShader->use();
            nuroShader->setMat4("M", M);
        }
        if (timeStepChanged)
            timeStepChanged = false;

        if (cameraChanged) {
            auto VP = proj * view;
            wormShader->use();
            wormShader->setMat4("VP", VP);
            nuroShader->use();
            nuroShader->setMat4("VP", VP);

            cameraChanged = false;
        }

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        if (drawWireFrame)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        if (wpd) {
            wormShader->use();
            wormShader->setVec4("color", glm::vec4{.2f, .3f, .4f, .5f});
            glBindVertexArray(wpd->GetVAO());
            glDrawArrays(GL_LINE_STRIP, timeStep * wormVertCnt, wormVertCnt);
            glBindVertexArray(0);
        }
        if (wnpd) {
            nuroShader->use();
            nuroShader->setVec4("color", glm::vec4{.3f, .1f, 1.f, .5f});
            glBindVertexArray(wnpd->GetVAO());
            glPointSize(3.f);
            glDrawArrays(GL_POINTS, timeStep * nuroVertCnt, nuroVertCnt);
            glBindVertexArray(0);
        }
        if (drawWireFrame)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
};

} // namespace kouek

#endif // !KOUEK_WORM_RENDERER_H
