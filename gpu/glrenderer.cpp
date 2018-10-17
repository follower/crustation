#include "glrenderer.h"


#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include <QScreen>
#include <QGuiApplication>


const unsigned int VRAM_WIDTH=1024;
const unsigned int VRAM_HEIGHT=512;


GLRenderer::GLRenderer(QWidget *parent) : QOpenGLWidget(parent) {
    this->setFixedSize(VRAM_WIDTH, VRAM_HEIGHT);
}


// via <http://doc.qt.io/qt-5/qtgui-openglwindow-example.html>
// (with a smattering of <https://www.ics.com/blog/fixed-function-modern-opengl-part-2-4>)

    static const char *vertexShaderSource =
        "attribute highp vec4 positionAttr;\n"
        "attribute lowp vec4 colorAttr;\n"
        "varying lowp vec4 col;\n"
        "uniform highp mat4 matrix;\n"
        "void main() {\n"
        "   col = colorAttr;\n"
        "   gl_Position = matrix * positionAttr;\n"
        "}\n";

    static const char *fragmentShaderSource =
        "varying lowp vec4 col;\n"
        "void main() {\n"
        "   gl_FragColor = col;\n"
        "}\n";

    static const char *textured_vertexShaderSource =
        "attribute highp vec4 positionAttr;\n"
        "attribute mediump vec4 texCoord;\n"
        "varying mediump vec4 texc;\n"
        "attribute lowp vec4 colorAttr;\n"
        "varying lowp vec4 col;\n"
        "uniform highp mat4 matrix;\n"
        "void main() {\n"
        "   col = colorAttr;\n"
        "   texc = texCoord;\n"
        "   gl_Position = matrix * positionAttr;\n"
        "}\n";

    static const char *textured_fragmentShaderSource =
        "varying lowp vec4 col;\n"
        "uniform sampler2D texture;\n"
        "varying mediump vec4 texc;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(texture, texc.st);\n"
        "}\n";


    void GLRenderer::initializeGL() {

        initializeOpenGLFunctions();

        glClearColor(QColor(Qt::gray).redF(), QColor(Qt::gray).greenF(), QColor(Qt::gray).blueF(), 1.0);

        vram_render_program = new QOpenGLShaderProgram(this);
        vram_render_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
        vram_render_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
        vram_render_program->link();
        positionAttr = vram_render_program->attributeLocation("positionAttr");
        colorAttr = vram_render_program->attributeLocation("colorAttr");
        matrixUniform = vram_render_program->uniformLocation("matrix");


        textured_render_program = new QOpenGLShaderProgram(this);
        textured_render_program->addShaderFromSourceCode(QOpenGLShader::Vertex, textured_vertexShaderSource);
        textured_render_program->addShaderFromSourceCode(QOpenGLShader::Fragment, textured_fragmentShaderSource);
        textured_render_program->link();
        textured_positionAttr = textured_render_program->attributeLocation("positionAttr");
        textured_colorAttr = textured_render_program->attributeLocation("colorAttr");
        textured_matrixUniform = textured_render_program->uniformLocation("matrix");

        textured_texCoordAttr = textured_render_program->attributeLocation("texCoord");

    }


    void GLRenderer::paintGL() {

        glClear(GL_COLOR_BUFFER_BIT);

        vram_render_program->bind();

        QMatrix4x4 matrix;

        // TODO: Handle this better?
        matrix.translate(-1, 1, 0);
        matrix.scale(float(VRAM_WIDTH)/VRAM_HEIGHT, 1.0f);
        matrix.ortho(-2.0f*VRAM_WIDTH/VRAM_HEIGHT, 2.0f*VRAM_WIDTH/VRAM_HEIGHT, -2.0f, 2.0f, -2.0f, 2.0f);
        matrix.scale(4.0f);
        matrix.scale(1, -1, 1); // Note: This works around different directions of Y-positive values between PSX GPU & OpenGL.

        vram_render_program->setUniformValue(matrixUniform, matrix);


        if (this->vertices.size() > 0) {

            glVertexAttribPointer(positionAttr, 2, GL_FLOAT, GL_FALSE, 0, &this->vertices[0]);
            glVertexAttribPointer(colorAttr, 3, GL_FLOAT, GL_FALSE, 0, &this->colors[0]);

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);

            glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());

            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(0);

        }

        vram_render_program->release();


        textured_render_program->bind();

        textured_render_program->setUniformValue(textured_matrixUniform, matrix);

        if ((this->vram_vertices_textured.vertices.size() > 0) && this->vram_vertices_textured.textures.size() > 0) {

            glVertexAttribPointer(textured_positionAttr, 2, GL_FLOAT, GL_FALSE, 0, &this->vram_vertices_textured.vertices[0]);
            glVertexAttribPointer(textured_colorAttr, 3, GL_FLOAT, GL_FALSE, 0, &this->vram_vertices_textured.colors[0]);
            glVertexAttribPointer(textured_texCoordAttr, 2, GL_FLOAT, GL_FALSE, 0, &this->vram_vertices_textured.texCoords[0]);

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);

            for (int texture_index = 0; texture_index < this->vram_vertices_textured.textures.size(); texture_index++) {
                this->vram_vertices_textured.textures[texture_index]->bind();
                glDrawArrays(GL_TRIANGLES, texture_index * 6, 6);
            }

            glDisableVertexAttribArray(2);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(0);

        }


        if ((this->screen_vertices_textured.vertices.size() > 0) && this->screen_vertices_textured.textures.size() > 0) {

            glVertexAttribPointer(textured_positionAttr, 2, GL_FLOAT, GL_FALSE, 0, &this->screen_vertices_textured.vertices[0]);
            glVertexAttribPointer(textured_colorAttr, 3, GL_FLOAT, GL_FALSE, 0, &this->screen_vertices_textured.colors[0]);
            glVertexAttribPointer(textured_texCoordAttr, 2, GL_FLOAT, GL_FALSE, 0, &this->screen_vertices_textured.texCoords[0]);

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);

            for (int texture_index = 0; texture_index < this->screen_vertices_textured.textures.size(); texture_index++) {
                this->screen_vertices_textured.textures[texture_index]->bind();
                glDrawArrays(GL_TRIANGLES, texture_index * 6, 6);
            }

            glDisableVertexAttribArray(2);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(0);

        }

        textured_render_program->release();

    }


    void GLRenderer::resizeGL(int w, int h) {

        const qreal retinaScale = devicePixelRatio();
        glViewport(0, 0, w * retinaScale, h * retinaScale);

        update();

    }


void GLRenderer::drawPolygon(GpuCommand *current_command, bool useItemColor) {

    if (this->resetRequested) {

        this->vertices.clear();
        this->colors.clear();

        this->resetRequested = false;
    }

    for (QVariant item: current_command->parameters) {
        if (item.canConvert<QPoint>()) {
            // Note: Scaling is by VRAM width/height, not by selected PSX screen width/height.
            // TODO: Do scaling in shader instead?
            this->vertices.append(QVector2D(item.value<QPoint>()) / QVector2D(VRAM_WIDTH, VRAM_HEIGHT));
        } else if (item.canConvert<QColor>()) {
            QColor theColor = item.value<QColor>();
            this->colors.append(QVector3D(theColor.redF(), theColor.greenF(), theColor.blueF()));
        }
    }

    // TODO: Handle this using OpenGL Elements/Indexes?
    if (current_command->command_value==GpuCommand::Gpu0_Opcodes::gp0_shaded_quad) {
        // Handle split from quad into two triangles.
        this->vertices.insert(this->vertices.size()-1, this->vertices.at(this->vertices.size()-3));
        this->vertices.insert(this->vertices.size()-1, this->vertices.at(this->vertices.size()-3));

        this->colors.insert(this->colors.size()-1, this->colors.at(this->colors.size()-3));
        this->colors.insert(this->colors.size()-1, this->colors.at(this->colors.size()-3));
    } else if (current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_monochrome_quad
               || current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_textured_quad) {

        // Handle split from quad into two triangles.
        this->vertices.insert(this->vertices.size()-1, this->vertices.at(this->vertices.size()-3));
        this->vertices.insert(this->vertices.size()-1, this->vertices.at(this->vertices.size()-3));

        this->colors.insert(this->colors.size(), 2+3, this->colors.last());
    }

}


void GLRenderer::drawTexture(GpuCommand *current_command) {

    auto position_vertex = current_command->parameters.first().toPoint();
    auto size_vertex = current_command->parameters.last().toPoint();

    // TODO: Remove duplicate/overlapping VRAM vertices/textures?

    // TODO: Handle this using OpenGL Elements/Indexes?

    this->vram_vertices_textured.addVertex(QVector2D(position_vertex) / QVector2D(VRAM_WIDTH, VRAM_HEIGHT), QVector2D(0, 1));
    this->vram_vertices_textured.addVertex((QVector2D(position_vertex) + QVector2D(size_vertex)) / QVector2D(VRAM_WIDTH, VRAM_HEIGHT), QVector2D(1, 0));
    this->vram_vertices_textured.addVertex((QVector2D(position_vertex) + QVector2D(size_vertex.x(), 0)) / QVector2D(VRAM_WIDTH, VRAM_HEIGHT), QVector2D(1, 1));


    this->vram_vertices_textured.addVertex(QVector2D(position_vertex) / QVector2D(VRAM_WIDTH, VRAM_HEIGHT), QVector2D(0, 1));
    this->vram_vertices_textured.addVertex((QVector2D(position_vertex) + QVector2D(size_vertex)) / QVector2D(VRAM_WIDTH, VRAM_HEIGHT), QVector2D(1, 0));
    this->vram_vertices_textured.addVertex((QVector2D(position_vertex) + QVector2D(0, size_vertex.y())) / QVector2D(VRAM_WIDTH, VRAM_HEIGHT), QVector2D(0, 0));


    this->vram_vertices_textured.textures.append(this->point_to_texture_lookup.value((position_vertex.x() << 16 | position_vertex.y())));

}


void GLRenderer::loadTexture(GpuCommand *current_command) {

    auto position_vertex = current_command->parameters.first().toPoint();


    this->textures_loaded.append(new QOpenGLTexture(current_command->texture->mirrored()));

    this->textures_loaded.last()->setMagnificationFilter(QOpenGLTexture::Nearest);

    //        this->point_to_texture_lookup[std::make_pair(position_vertex.x(), position_vertex.y())] = this->textures.last();
    this->point_to_texture_lookup[(position_vertex.x() << 16 | position_vertex.y())] = this->textures_loaded.last();

    this->drawTexture(current_command);

}


void GLRenderer::doRender() {
    update();
}


void GLRenderer::clear() {
    this->resetRequested = true;
}
