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
        QVector<QVector4D> colors;
        QVector<QVector2D> texCoords;

        void addVertex(QVector2D vertex, QVector2D textureCoords, QVector4D color = QVector4D(1.0, 0, 0, 1.0)) {

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

    void drawTexture(GpuCommand *current_command);
    void loadTexture(GpuCommand *current_command);

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
        QVector<QVector4D> colors;


        GLuint textured_texCoordAttr;

        GLuint textured_positionAttr;
        GLuint textured_colorAttr;
        GLuint textured_matrixUniform;


        QOpenGLShaderProgram *textured_render_program;

        QVector<QOpenGLTexture *> textures_loaded; // Those stored in VRAM.

        VertexBundle screen_vertices_textured; // For current PSX "screen" display.
        VertexBundle vram_vertices_textured; // For our visualisation of VRAM.

        // TODO: Handle the key side better...
        QHash<quint32, QOpenGLTexture *> point_to_texture_lookup;

        bool resetRequested = false;

};

#endif // GLRENDERER_H
