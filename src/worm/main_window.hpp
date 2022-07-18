#ifndef KOUEK_MAIN_WINDOW_H
#define KOUEK_MAIN_WINDOW_H

#include <memory>

#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qwidget.h>

#include <ui_main_window.h>

#include <cmake_in.h>
#include <util/FPS_camera.h>

#include "gl_view.hpp"

namespace Ui {
class MainWindow;
}

namespace kouek {
class MainWindow : public QWidget {
    Q_OBJECT

  private:
    Ui::MainWindow *ui;
    GLView *glView;
    std::shared_ptr<WormPositionData> wpd;
    std::shared_ptr<WormNeuronPositionData> wnpd;
    std::shared_ptr<WormRenderer> renderer;

    bool is3D = false;
    float fov = 60.f;
    FPSCamera camera;

  public:
    explicit MainWindow(QWidget *parent = Q_NULLPTR)
        : QWidget(parent), ui(new Ui::MainWindow),
          camera(glm::vec3{0, 0, 1.f}, glm::zero<glm::vec3>()) {
        ui->setupUi(this);

        glView = new GLView();
        ui->groupBoxView->layout()->addWidget(glView);

        ui->groupBoxReg->setEnabled(false);
        ui->groupBoxWNPD->setEnabled(false);
        ui->groupBoxLighting->setEnabled(false);

        // slots
        connect(ui->toolButtonBroswerWPD, &QToolButton::clicked, [&]() {
            wpd.reset();
            wnpd.reset();
            ui->groupBoxWNPD->setEnabled(false);
            ui->groupBoxReg->setEnabled(false);
            auto path = QFileDialog::getOpenFileName(
                this, tr("Open Worm Position Data"));
            if (path.isEmpty())
                return;
            try {
                glView->makeCurrent();
                wpd = std::make_shared<WormPositionData>(path.toStdString());

                ui->labelWPDPath->setText(path);
                ui->horizontalSliderTiemStep->setMaximum(
                    wpd->GetVerts().size() - 1);
                ui->labelMinTimeStep->setText(QString::number(0));
                ui->labelMaxTimeStep->setText(
                    QString::number(wpd->GetVerts().size() - 1));
                ui->groupBoxWNPD->setEnabled(true);

                // cascading slots
                ui->radioButtonViewIn3D->clicked(
                    ui->radioButtonViewIn3D->isChecked());

                renderer->SetRenderTarget(WormRenderer::RenderTarget::Worm);
                glView->update();
            } catch (std::exception &e) {
                ui->labelWPDPath->setText(e.what());
            }
        });
        connect(ui->toolButtonBroswerWNPD, &QToolButton::clicked, [&]() {
            wnpd.reset();
            auto path = QFileDialog::getOpenFileName(
                this, tr("Open Worm Neuron Position Data"));
            if (path.isEmpty())
                return;
            try {
                glView->makeCurrent();
                wnpd = std::make_shared<WormNeuronPositionData>(
                    path.toStdString(), wpd);

                ui->labelWNPDPath->setText(path);
                ui->groupBoxReg->setEnabled(true);
                
                // cascading slots
                ui->radioButtonViewIn3D->clicked(
                    ui->radioButtonViewIn3D->isChecked());
                ui->pushButtonCurveFit->clicked();

                glView->update();
            } catch (std::exception &e) {
                ui->labelWNPDPath->setText(e.what());
            }
        });
        connect(ui->pushButtonCurveFit, &QPushButton::clicked, [&]() {
            glView->makeCurrent();
            wnpd->PolyCurveFitWith(ui->spinBoxCurveFitOrder->value(),
                                   ui->spinBoxRANSAC_ST->value(),
                                   ui->doubleSpinBoxRANSAC_IHW->value());
            renderer->SetRenderTarget(WormRenderer::RenderTarget::Neuron);
            glView->update();
        });
        connect(ui->radioButtonViewIn3D, &QRadioButton::clicked,
                [&](bool checked) {
                    is3D = checked;
                    ui->groupBoxLighting->setEnabled(is3D);

                    std::pair<WormRenderer::RenderTarget, bool> lastRT{
                        WormRenderer::RenderTarget::Worm, false};
                    if (renderer)
                        lastRT = {renderer->GetRenderTarget(), true};
                    if (checked) {
                        renderer = std::make_shared<WormRenderer3D>();
                        dynamic_cast<WormRenderer3D *>(renderer.get())
                            ->SetDivisionNum(16);
                    } else
                        renderer = std::make_shared<WormRenderer2D>();
                    renderer->SetWormPositionDat(wpd);
                    renderer->SetWormNeuronPositionDat(wnpd);
                    if (lastRT.second)
                        renderer->SetRenderTarget(lastRT.first);
                    glView->SetRenderer(renderer);
                    
                    // cascading slots
                    ui->radioButtonViewWireFrame->clicked(
                        ui->radioButtonViewWireFrame->isChecked());
                    ui->comboBoxSceneMode->currentIndexChanged(
                        ui->comboBoxSceneMode->currentIndex());
                    ui->horizontalSliderTiemStep->valueChanged(
                        ui->horizontalSliderTiemStep->value());
                    if (is3D)
                        syncFromLightParam();
                    glView->GLResized(glView->width(), glView->height());
                    glView->update();
                });
        connect(ui->radioButtonViewWireFrame, &QRadioButton::clicked,
                [&](bool checked) {
                    if (renderer)
                        renderer->SetDrawWireFrame(checked);
                    glView->update();
                });
        connect(ui->comboBoxSceneMode,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                [&](int idx) {
                    if (renderer)
                        renderer->SetSceneMode(
                            static_cast<WormRenderer::SceneMode>(idx));
                    glView->update();
                });
        connect(ui->horizontalSliderTiemStep, &QSlider::valueChanged,
                [&](int val) {
                    if (renderer)
                        renderer->SetTimeStep(val);
                    ui->labelTimeStep->setText(QString::number(val));
                    glView->update();
                });
        connect(ui->doubleSpinBoxAmbientStrength,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    WormRenderer3D *ptr =
                        dynamic_cast<WormRenderer3D *>(renderer.get());
                    auto param = ptr->GetLightParam();
                    param.ambientStrength = val;
                    ptr->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightR,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    WormRenderer3D *ptr =
                        dynamic_cast<WormRenderer3D *>(renderer.get());
                    auto param = ptr->GetLightParam();
                    param.lightColor.r = val;
                    ptr->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightG,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    WormRenderer3D *ptr =
                        dynamic_cast<WormRenderer3D *>(renderer.get());
                    auto param = ptr->GetLightParam();
                    param.lightColor.g = val;
                    ptr->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightB,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    WormRenderer3D *ptr =
                        dynamic_cast<WormRenderer3D *>(renderer.get());
                    auto param = ptr->GetLightParam();
                    param.lightColor.b = val;
                    ptr->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightX,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    WormRenderer3D *ptr =
                        dynamic_cast<WormRenderer3D *>(renderer.get());
                    auto param = ptr->GetLightParam();
                    param.lightPos.x = val;
                    ptr->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightY,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    WormRenderer3D *ptr =
                        dynamic_cast<WormRenderer3D *>(renderer.get());
                    auto param = ptr->GetLightParam();
                    param.lightPos.y = val;
                    ptr->SetLightParam(param);
                    glView->update();
                });
        connect(ui->doubleSpinBoxLightZ,
                QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [&](double val) {
                    WormRenderer3D *ptr =
                        dynamic_cast<WormRenderer3D *>(renderer.get());
                    auto param = ptr->GetLightParam();
                    param.lightPos.z = val;
                    ptr->SetLightParam(param);
                    glView->update();
                });
        connect(glView, &GLView::GLResized, [&](int w, int h) {
            if (!renderer)
                return;
            if (is3D)
                dynamic_cast<WormRenderer3D *>(renderer.get())
                    ->SetCamera(camera.getViewMat(),
                                glm::perspectiveFov(glm::radians(fov), (float)w,
                                                    (float)h, .001f, 10.f));
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
    }

  private:
    inline void syncFromLightParam() {
        ui->doubleSpinBoxAmbientStrength->valueChanged(
            ui->doubleSpinBoxAmbientStrength->value());
        ui->doubleSpinBoxLightR->valueChanged(ui->doubleSpinBoxLightR->value());
        ui->doubleSpinBoxLightG->valueChanged(ui->doubleSpinBoxLightG->value());
        ui->doubleSpinBoxLightB->valueChanged(ui->doubleSpinBoxLightB->value());
        ui->doubleSpinBoxLightX->valueChanged(ui->doubleSpinBoxLightX->value());
        ui->doubleSpinBoxLightY->valueChanged(ui->doubleSpinBoxLightY->value());
        ui->doubleSpinBoxLightZ->valueChanged(ui->doubleSpinBoxLightZ->value());
    }
};
} // namespace kouek

#endif // !KOUEK_MAIN_WINDOW_H
