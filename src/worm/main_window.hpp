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

        // slots
        connect(ui->toolButtonBroswerWPD, &QToolButton::clicked, [&]() {
            ui->toolButtonBroswerWNPD->setEnabled(false);
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

                ui->toolButtonBroswerWNPD->setEnabled(true);
                ui->radioButtonViewIn3D->clicked(
                    ui->radioButtonViewIn3D->isChecked());
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

                ui->labelWNPDPath->setText(path);
                ui->radioButtonViewIn3D->clicked(
                    ui->radioButtonViewIn3D->isChecked());
            } catch (std::exception &e) {
                ui->labelWNPDPath->setText(e.what());
            }
        });
        connect(ui->radioButtonViewIn3D, &QRadioButton::clicked,
                [&](bool checked) {
                    is3D = checked;
                    if (checked) {
                        renderer = std::make_shared<WormRenderer3D>();
                        dynamic_cast<WormRenderer3D *>(renderer.get())
                            ->SetDivisionNum(16);
                    } else
                        renderer = std::make_shared<WormRenderer2D>();
                    renderer->SetWormPositionDat(wpd);
                    renderer->SetWormNeuronPositionDat(wnpd);
                    glView->SetRenderer(renderer);
                    // cascading slots
                    ui->radioButtonViewWireFrame->clicked(
                        ui->radioButtonViewWireFrame->isChecked());
                    ui->comboBoxSceneMode->currentIndexChanged(
                        ui->comboBoxSceneMode->currentIndex());
                    ui->horizontalSliderTiemStep->valueChanged(
                        ui->horizontalSliderTiemStep->value());
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
        connect(glView, &GLView::GLResized, [&](int w, int h) {
            if (!renderer)
                return;
            if (is3D)
                dynamic_cast<WormRenderer3D *>(renderer.get())
                    ->SetCamera(camera.getViewMat(),
                                glm::perspectiveFov(glm::radians(fov), (float)w,
                                                    (float)h, .1f, 10.f));
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
};
} // namespace kouek

#endif // !KOUEK_MAIN_WINDOW_H
