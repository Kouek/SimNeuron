#ifndef KOUEK_MAIN_WINDOW_H
#define KOUEK_MAIN_WINDOW_H

#include <memory>

#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qwidget.h>

#include <ui_main_window.h>

#include <cmake_in.h>
#include <util/FPS_camera.h>
#include <util/math.h>

#include "gl_view.hpp"

namespace Ui {
class MainWindow;
}

namespace kouek {
class MainWindow : public QWidget {
    Q_OBJECT

  private:
    static constexpr float N_CLIP = .001f;
    static constexpr float F_CLIP = 10.F;

    Ui::MainWindow *ui;
    GLView *glView;

    std::shared_ptr<WormPositionData> wpd;
    std::shared_ptr<WormNeuronPositionData> wnpd;
    std::shared_ptr<WormRenderer> renderer;

    bool isSelectingInliers = false;
    float fov = 90.f;
    glm::mat3 wnpdModelRevRot;
    glm::mat4 proj, unProj, wnpdModelRev;
    std::array<glm::vec2, 2> inliersSelectFrame;
    FPSCamera camera;

  public:
    explicit MainWindow(QWidget *parent = Q_NULLPTR)
        : QWidget(parent), ui(new Ui::MainWindow),
          camera(glm::vec3{0, 0, 1.f}, glm::zero<glm::vec3>()) {
        ui->setupUi(this);

        ui->groupBoxView->setLayout(new QVBoxLayout);
        glView = new GLView();
        ui->groupBoxView->layout()->addWidget(glView);

        ui->groupBoxWNPD->setEnabled(false);
        ui->groupBoxReg->setEnabled(false);
        ui->groupBoxRendering->setEnabled(false);
        ui->groupBoxTimeStep->setEnabled(false);
        ui->groupBoxLighting->setEnabled(false);

        // slots
        connect(ui->toolButtonBroswerWPD, &QToolButton::clicked, [&]() {
            auto path = QFileDialog::getOpenFileName(
                this, tr("Open Worm Position Data"));
            if (path.isEmpty())
                return;
            try {
                glView->makeCurrent();
                wpd = std::make_shared<WormPositionData>(path.toStdString());

                wnpd.reset();
                ui->labelWPDPath->setText(path);
                ui->horizontalSliderTiemStep->setMaximum(
                    wpd->GetVerts().size() - 1);
                ui->labelMinTimeStep->setText(QString::number(0));
                ui->labelMaxTimeStep->setText(
                    QString::number(wpd->GetVerts().size() - 1));

                ui->groupBoxWNPD->setEnabled(true);
                ui->groupBoxReg->setEnabled(false);
                ui->groupBoxRendering->setEnabled(true);
                ui->groupBoxTimeStep->setEnabled(true);
                ui->groupBoxLighting->setEnabled(true);

                renderer->SetWormPositionDat(wpd);
                renderer->SetRenderTarget(WormRenderer::RenderTarget::Worm);

                glView->GLResized(glView->width(), glView->height());
                glView->update();
            } catch (std::exception &e) {
                ui->labelWPDPath->setText(e.what());
            }
        });
        connect(ui->toolButtonBroswerWNPD, &QToolButton::clicked, [&]() {
            auto path = QFileDialog::getOpenFileName(
                this, tr("Open Worm Neuron Position Data"));
            if (path.isEmpty())
                return;
            try {
                glView->makeCurrent();
                wnpd = std::make_shared<WormNeuronPositionData>(
                    path.toStdString(), wpd);
                {
                    auto [min, max] = wnpd->GetPosRange();
                    glm::vec3 offset{-.5f * (max + min)};
                    offset.z = 0;
                    glm::vec2 delta = max - min;
                    glm::vec3 scale{2.f / std::max({delta.x, delta.y})};
                    auto M = glm::scale(glm::identity<glm::mat4>(), scale) *
                             glm::translate(glm::identity<glm::mat4>(), offset);
                    wnpdModelRev = glm::inverse(M);
                    wnpdModelRevRot = wnpdModelRev;
                }
                renderer->SetWormNeuronPositionDat(wnpd);
                renderer->SetRenderTarget(
                    WormRenderer::RenderTarget::NeuronReg);
                static constexpr auto NURO_HF_WID = .01f;
                renderer->SetNeuronHalfWidth(NURO_HF_WID);

                syncFromWormComponents();

                ui->labelWNPDPath->setText(path);
                ui->groupBoxReg->setEnabled(true);
                ui->tabWidgetReg->setCurrentIndex(0); // Neuron Registration tab
                ui->comboBoxSceneMode->setCurrentIndex(
                    static_cast<int>(WormRenderer::SceneMode::Full));
                ui->doubleSpinBoxNuroHfWid->setValue(NURO_HF_WID);
                ui->groupBoxRendering->setEnabled(false);
                ui->groupBoxTimeStep->setEnabled(false);
                ui->groupBoxLighting->setEnabled(false);

                glView->update();
            } catch (std::exception &e) {
                ui->labelWNPDPath->setText(e.what());
            }
        });
        connect(ui->tabWidgetReg, &QTabWidget::currentChanged, [&](int idx) {
            if (idx == 0)
                renderer->SetRenderTarget(
                    WormRenderer::RenderTarget::NeuronReg);
            else
                renderer->SetRenderTarget(WormRenderer::RenderTarget::WormReg);
            glView->update();
        });
        connect(
            ui->pushButtonSlctInlierOrFitCurve, &QPushButton::clicked, [&]() {
                if (renderer->GetRenderTarget() !=
                    WormRenderer::RenderTarget::NeuronReg)
                    return;
                isSelectingInliers = !isSelectingInliers;
                static constexpr char *SELECT_INLIERS_TXT = "Select Inliers";
                static constexpr char *POLY_FIT_CURVE_TXT = "Poly Fit Curve";

                glView->makeCurrent();
                if (isSelectingInliers) {
                    ui->pushButtonSlctInlierOrFitCurve->setText(
                        POLY_FIT_CURVE_TXT);
                    wnpd->ClearCurve();
                    wnpd->UnselectInliers();
                }
                else {
                    ui->pushButtonSlctInlierOrFitCurve->setText(
                        SELECT_INLIERS_TXT);
                    wnpd->PolyCurveFitWith(ui->spinBoxCurveFitOrder->value());
                    static constexpr std::array<glm::vec2, 2> zeroes{
                        glm::zero<glm::vec2>()};
                    renderer->SetFrameVerts(zeroes);
                }
                glView->update();
            });
        connect(ui->doubleSpinBoxHeadStart,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    glView->makeCurrent();
                    syncFromWormComponents();
                    glView->update();
                });
        connect(ui->doubleSpinBoxHeadEnd,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    glView->makeCurrent();
                    syncFromWormComponents();
                    glView->update();
                });
        connect(ui->doubleSpinBoxVCStart,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    glView->makeCurrent();
                    syncFromWormComponents();
                    glView->update();
                });
        connect(ui->doubleSpinBoxVCEnd,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    glView->makeCurrent();
                    syncFromWormComponents();
                    glView->update();
                });
        connect(ui->doubleSpinBoxTailStart,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    glView->makeCurrent();
                    syncFromWormComponents();
                    glView->update();
                });
        connect(ui->doubleSpinBoxTailEnd,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    glView->makeCurrent();
                    syncFromWormComponents();
                    glView->update();
                });
        connect(ui->pushButtonReg, &QPushButton::clicked, [&]() {
            ui->groupBoxReg->setEnabled(false);
            ui->groupBoxRendering->setEnabled(true);
            ui->groupBoxTimeStep->setEnabled(true);
            ui->groupBoxLighting->setEnabled(true);

            glView->makeCurrent();
            wnpd->RegisterWithWPD();
            renderer->SetRenderTarget(
                WormRenderer::RenderTarget::WormAndNeuron);
            glView->update();
        });
        connect(ui->radioButtonViewWireFrame, &QRadioButton::clicked,
                [&](bool checked) {
                    renderer->SetDrawWireFrame(checked);
                    glView->update();
                });
        connect(ui->doubleSpinBoxFrontFaceOpacity,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    renderer->SetFrontFaceOpacity(val);
                    glView->update();
                });
        connect(ui->doubleSpinBoxNuroHfWid,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    glView->makeCurrent();
                    renderer->SetNeuronHalfWidth(val);
                    glView->update();
                });
        connect(ui->comboBoxSceneMode,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                [&](int idx) {
                    renderer->SetSceneMode(
                        static_cast<WormRenderer::SceneMode>(idx));
                    ui->horizontalSliderTiemStep->valueChanged(
                        ui->horizontalSliderTiemStep->value());
                    glView->update();
                });
        connect(ui->horizontalSliderTiemStep, &QSlider::valueChanged,
                [&](int val) {
                    renderer->SetTimeStep(val);
                    ui->labelTimeStep->setText(QString::number(val));
                    glView->update();
                });
        connect(ui->doubleSpinBoxAmbientStrength,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    auto param = renderer->GetLightParam();
                    param.ambientStrength = val;
                    renderer->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightR,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    auto param = renderer->GetLightParam();
                    param.lightColor.r = val;
                    renderer->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightG,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    auto param = renderer->GetLightParam();
                    param.lightColor.g = val;
                    renderer->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightB,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    auto param = renderer->GetLightParam();
                    param.lightColor.b = val;
                    renderer->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightX,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    auto param = renderer->GetLightParam();
                    param.lightPos.x = val;
                    renderer->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightY,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    auto param = renderer->GetLightParam();
                    param.lightPos.y = val;
                    renderer->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightZ,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    auto param = renderer->GetLightParam();
                    param.lightPos.z = val;
                    renderer->SetLightParam(param);
                    glView->update();
                });
        connect(glView, &GLView::GLInited, [&]() {
            // no need to call makeCurrent()
            renderer = std::make_shared<WormRenderer>();
            renderer->SetDivisionNum(16);
            syncFromRenderPamram();
            syncFromLightParam();
            glView->SetRenderer(renderer);
        });
        connect(glView, &GLView::GLResized, [&](int w, int h) {
            proj = glm::perspectiveFov(glm::radians(fov), (float)w, (float)h,
                                       N_CLIP, F_CLIP);
            unProj = Math::inverseProjective(proj);
            renderer->SetCamera(camera.getViewMat(), proj);
        });
        connect(glView, &GLView::KeyPressed, [&](int key, int funcKey) {
            glView->makeCurrent();

            constexpr static float MOV_SENSITY = .01f;
            constexpr static float ROT_SENSITY = 1.f;
            switch (key) {
            case Qt::Key_Up:
                if (funcKey == Qt::Key_Shift)
                    camera.move(0, +MOV_SENSITY, 0);
                else if (funcKey == Qt::Key_Control)
                    camera.rotate(0, +ROT_SENSITY);
                else
                    camera.move(0, 0, +MOV_SENSITY);
                break;
            case Qt::Key_Down:
                if (funcKey == Qt::Key_Shift)
                    camera.move(0, -MOV_SENSITY, 0);
                else if (funcKey == Qt::Key_Control)
                    camera.rotate(0, -ROT_SENSITY);
                else
                    camera.move(0, 0, -MOV_SENSITY);
                break;
            case Qt::Key_Left:
                if (funcKey == Qt::Key_Control)
                    camera.rotate(+ROT_SENSITY, 0);
                else
                    camera.move(-MOV_SENSITY, 0, 0);
                break;
            case Qt::Key_Right:
                if (funcKey == Qt::Key_Control)
                    camera.rotate(-ROT_SENSITY, 0);
                else
                    camera.move(+MOV_SENSITY, 0, 0);
                break;
            }
            glView->GLResized(glView->width(), glView->height());
            glView->update();
        });
        connect(glView, &GLView::MousePressed, [&](const glm::vec2 &normPos) {
            if (!isSelectingInliers)
                return;
            inliersSelectFrame[0] = normPos;
        });
        connect(glView, &GLView::MouseMoved, [&](const glm::vec2 &normPos) {
            if (!isSelectingInliers)
                return;
            inliersSelectFrame[1] = normPos;

            std::array<glm::vec2, 2> minMax;
            for (uint8_t xy = 0; xy < 2; ++xy) {
                if (inliersSelectFrame[0][xy] < inliersSelectFrame[1][xy]) {
                    minMax[0][xy] = inliersSelectFrame[0][xy];
                    minMax[1][xy] = inliersSelectFrame[1][xy];
                } else {
                    minMax[0][xy] = inliersSelectFrame[1][xy];
                    minMax[1][xy] = inliersSelectFrame[0][xy];
                }
                minMax[0][xy] = 2.f * minMax[0][xy] - 1.f;
                minMax[1][xy] = 2.f * minMax[1][xy] - 1.f;
            }
            glView->makeCurrent();
            renderer->SetFrameVerts(minMax);
            glView->update();
        });
        connect(glView, &GLView::MouseReleased, [&](const glm::vec2 &normPos) {
            if (!isSelectingInliers)
                return;
            inliersSelectFrame[1] = normPos;
            for (uint8_t xy = 0; xy < 2; ++xy) {
                if (inliersSelectFrame[0][xy] > inliersSelectFrame[1][xy])
                    std::swap(inliersSelectFrame[0][xy],
                              inliersSelectFrame[1][xy]);
                inliersSelectFrame[0][xy] =
                    2.f * inliersSelectFrame[0][xy] - 1.f;
                inliersSelectFrame[1][xy] =
                    2.f * inliersSelectFrame[1][xy] - 1.f;
            }
            if (inliersSelectFrame[0].x == inliersSelectFrame[1].x ||
                inliersSelectFrame[0].y == inliersSelectFrame[1].y)
                return;

            glView->makeCurrent();
            auto [R, F, U, P] = camera.getRFUP();
            glm::mat3 cameraRot{R.x, R.y, R.z, U.x, U.y, U.z, -F.x, -F.y, -F.z};
            glm::vec4 tmp{0, 0, 1.f, 1.f};
            glm::vec3 newP = wnpdModelRev * glm::vec4{P, 1.f};
            glm::vec3 newF = wnpdModelRevRot * F;
            std::array<glm::vec3, 4> drcs;
            for (uint8_t drcIdx = 0; drcIdx < 4; ++drcIdx) {
                tmp.x = inliersSelectFrame[(drcIdx & 0x1) ? 1 : 0].x;
                tmp.y = inliersSelectFrame[(drcIdx & 0x2) ? 1 : 0].y;
                drcs[drcIdx] = unProj * tmp;
                drcs[drcIdx] =
                    glm::normalize(wnpdModelRevRot * cameraRot * drcs[drcIdx]);
            }
            Frustum frustum(newP, newF, N_CLIP, F_CLIP, drcs[0], drcs[2],
                            drcs[1], drcs[3]);
            wnpd->SelectAndAppendInliers(frustum);
            glView->update();
        });
    }

  private:
    inline void syncFromRenderPamram() {
        ui->radioButtonViewWireFrame->clicked(
            ui->radioButtonViewWireFrame->isChecked());
        ui->doubleSpinBoxFrontFaceOpacity->valueChanged(
            ui->doubleSpinBoxFrontFaceOpacity->value());
        ui->doubleSpinBoxNuroHfWid->valueChanged(
            ui->doubleSpinBoxNuroHfWid->value());
        ui->comboBoxSceneMode->setCurrentIndex(
            ui->comboBoxSceneMode->currentIndex());
    }
    inline void syncFromLightParam() {
        std::array arrs{
            ui->doubleSpinBoxAmbientStrength, ui->doubleSpinBoxLightR,
            ui->doubleSpinBoxLightG,          ui->doubleSpinBoxLightB,
            ui->doubleSpinBoxLightX,          ui->doubleSpinBoxLightY,
            ui->doubleSpinBoxLightZ};
        for (auto ptr : arrs)
            ptr->valueChanged(ptr->value());
    }
    inline void syncFromWormComponents() {
        std::array arrs{ui->doubleSpinBoxHeadStart, ui->doubleSpinBoxHeadEnd,
                        ui->doubleSpinBoxVCStart,   ui->doubleSpinBoxVCEnd,
                        ui->doubleSpinBoxTailStart, ui->doubleSpinBoxTailEnd};
        for (uint8_t idx = 1; idx < arrs.size(); ++idx) {
            auto lft = arrs[idx - 1]->value();
            auto rht = arrs[idx]->value();
            if (lft > rht)
                arrs[idx]->setValue(lft);
        }
        wpd->SetComponentRatio(WormPositionData::Component::Head,
                               ui->doubleSpinBoxHeadStart->value(),
                               ui->doubleSpinBoxHeadEnd->value());
        wpd->SetComponentRatio(WormPositionData::Component::VentralCord,
                               ui->doubleSpinBoxVCStart->value(),
                               ui->doubleSpinBoxVCEnd->value());
        wpd->SetComponentRatio(WormPositionData::Component::Tail,
                               ui->doubleSpinBoxTailStart->value(),
                               ui->doubleSpinBoxTailEnd->value());
    }
};
} // namespace kouek

#endif // !KOUEK_MAIN_WINDOW_H
