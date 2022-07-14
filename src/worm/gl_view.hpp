#ifndef KOUEK_GL_VIEW_H
#define KOUEK_GL_VIEW_H

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <QtWidgets/qopenglwidget.h>
#include <QtGui/qevent.h>

#include "worm_renderer.hpp"

namespace kouek {
class GLView : public QOpenGLWidget {
    Q_OBJECT

  private:
    int funcKey = -1;
    std::shared_ptr<WormRenderer> renderer;

  public:
    explicit GLView(QWidget *parent = Q_NULLPTR) : QOpenGLWidget(parent) {
        setFocusPolicy(Qt::StrongFocus);
        setCursor(Qt::CrossCursor);

        QSurfaceFormat surfaceFmt;
        surfaceFmt.setDepthBufferSize(24);
        surfaceFmt.setStencilBufferSize(8);
        surfaceFmt.setVersion(4, 5);
        surfaceFmt.setProfile(QSurfaceFormat::CoreProfile);
        setFormat(surfaceFmt);
    }
    void SetRenderer(std::shared_ptr<WormRenderer> renderer) {
        this->renderer = renderer;
    }

  signals:
    void GLResized(int w, int h);
    void KeyPressed(int key, int funcKey = -1);

  protected:
    void initializeGL() override {
        int hasInit = gladLoadGL();
        assert(hasInit);

        glClearColor(0.f, 0.f, 0.f, 1.f);
    }
    void resizeGL(int w, int h) override { emit GLResized(w, h); }
    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (renderer)
            renderer->Render();
    }

  protected:
    void keyPressEvent(QKeyEvent *e) {
        switch (e->key()) {
        case Qt::Key_Shift:
            funcKey = Qt::Key_Shift;
            break;
        case Qt::Key_Control:
            funcKey = Qt::Key_Control;
            break;
        default:
            emit KeyPressed(e->key(), funcKey);
        }
    }
    void keyReleaseEvent(QKeyEvent* e) {
        switch (e->key()) {
        case Qt::Key_Shift:
        case Qt::Key_Control:
            funcKey = -1;
            break;
        }
    }
};
} // namespace kouek

#endif // !KOUEK_GL_VIEW_H
