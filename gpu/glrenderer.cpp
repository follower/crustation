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


void GLRenderer::doRender() {
    update();
}


void GLRenderer::clear() {
    this->resetRequested = true;
}
