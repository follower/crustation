#ifndef GLRENDERER_H
#define GLRENDERER_H

#include "command.h"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>



class GLRenderer : public QOpenGLWidget, protected QOpenGLFunctions {
public:
    GLRenderer(QWidget *parent);

    void drawPolygon(GpuCommand *current_command, bool useItemColor = true);
    void doRender();
    void clear();

protected:
    // QOpenGLWidget interface
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    private:
        GLuint m_posAttr;
        GLuint m_colAttr;
        GLuint m_matrixUniform;

        QOpenGLShaderProgram *m_program;

        QVector<QVector2D> vertices;
        QVector<QVector3D> colors;

        bool resetRequested = false;

};

#endif // GLRENDERER_H
