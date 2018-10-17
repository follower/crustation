#ifndef GLRENDERER_H
#define GLRENDERER_H

#include "command.h"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWidget>


class VertexBundle {

public: //
        QVector<QOpenGLTexture *> textures;

        QVector<QVector2D> vertices;
        QVector<QVector3D> colors;
        QVector<QVector2D> texCoords;

        void addVertex(QVector2D vertex, QVector2D textureCoords, QVector3D color = QVector3D(1.0, 0, 0)) {

            this->vertices.append(vertex);
            this->texCoords.append(textureCoords);
            this->colors.append(color);

        }
};


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
        GLuint positionAttr;
        GLuint colorAttr;
        GLuint matrixUniform;

        QOpenGLShaderProgram *vram_render_program;

        QVector<QVector2D> vertices;
        QVector<QVector3D> colors;

        bool resetRequested = false;

};

#endif // GLRENDERER_H
