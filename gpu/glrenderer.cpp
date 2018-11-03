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


        // TODO: Remove duplication between the following two sections...

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


        // TODO: Remove duplication between the following three sections...

        if (this->vertices.size() > 0) {

            glVertexAttribPointer(positionAttr, 2, GL_FLOAT, GL_FALSE, 0, &this->vertices[0]);
            glVertexAttribPointer(colorAttr, 4, GL_FLOAT, GL_FALSE, 0, &this->colors[0]);

            glEnableVertexAttribArray(positionAttr);
            glEnableVertexAttribArray(colorAttr);

            glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());

            glDisableVertexAttribArray(colorAttr);
            glDisableVertexAttribArray(positionAttr);

        }

        vram_render_program->release();


        textured_render_program->bind();

        textured_render_program->setUniformValue(textured_matrixUniform, matrix);


        if ((this->vram_vertices_textured.vertices.size() > 0) && this->vram_vertices_textured.textures.size() > 0) {

            glVertexAttribPointer(textured_positionAttr, 2, GL_FLOAT, GL_FALSE, 0, &this->vram_vertices_textured.vertices[0]);
            glVertexAttribPointer(textured_colorAttr, 4, GL_FLOAT, GL_FALSE, 0, &this->vram_vertices_textured.colors[0]);
            glVertexAttribPointer(textured_texCoordAttr, 2, GL_FLOAT, GL_FALSE, 0, &this->vram_vertices_textured.texCoords[0]);

            glEnableVertexAttribArray(textured_positionAttr);
            glEnableVertexAttribArray(textured_colorAttr);
            glEnableVertexAttribArray(textured_texCoordAttr);

            for (int texture_index = 0; texture_index < this->vram_vertices_textured.textures.size(); texture_index++) {
                this->vram_vertices_textured.textures[texture_index]->bind();
                glDrawArrays(GL_TRIANGLES, texture_index * 6, 6);
            }

            glDisableVertexAttribArray(textured_texCoordAttr);
            glDisableVertexAttribArray(textured_colorAttr);
            glDisableVertexAttribArray(textured_positionAttr);

        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if ((this->screen_vertices_textured.vertices.size() > 0) && this->screen_vertices_textured.textures.size() > 0) {

            glVertexAttribPointer(textured_positionAttr, 2, GL_FLOAT, GL_FALSE, 0, &this->screen_vertices_textured.vertices[0]);
            glVertexAttribPointer(textured_colorAttr, 4, GL_FLOAT, GL_FALSE, 0, &this->screen_vertices_textured.colors[0]);
            glVertexAttribPointer(textured_texCoordAttr, 2, GL_FLOAT, GL_FALSE, 0, &this->screen_vertices_textured.texCoords[0]);

            glEnableVertexAttribArray(textured_positionAttr);
            glEnableVertexAttribArray(textured_colorAttr);
            glEnableVertexAttribArray(textured_texCoordAttr);

            for (int texture_index = 0; texture_index < this->screen_vertices_textured.textures.size(); texture_index++) {
                this->screen_vertices_textured.textures[texture_index]->bind();
                glDrawArrays(GL_TRIANGLES, texture_index * 6, 6);
            }

            glDisableVertexAttribArray(textured_texCoordAttr);
            glDisableVertexAttribArray(textured_colorAttr);
            glDisableVertexAttribArray(textured_positionAttr);

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

        this->screen_vertices_textured.textures.clear();
        this->screen_vertices_textured.vertices.clear();
        this->screen_vertices_textured.colors.clear();
        this->screen_vertices_textured.texCoords.clear();

        this->resetRequested = false;
    }

    for (QVariant item: current_command->parameters) {
        if (item.canConvert<QPoint>()) {
            // Note: Scaling is by VRAM width/height, not by selected PSX screen width/height.
            // TODO: Do scaling in shader instead?
            this->vertices.append(QVector2D(item.value<QPoint>()) / QVector2D(VRAM_WIDTH, VRAM_HEIGHT));
        } else if (item.canConvert<QColor>()) {
            QColor theColor = item.value<QColor>();
            if (current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_textured_quad) {
                // Hacky way to make the polygon we draw intially invisible.
                // TODO: Handle this somewhere else?
                theColor.setAlphaF(0.0);
            }
            this->colors.append(QVector4D(theColor.redF(), theColor.greenF(), theColor.blueF(), theColor.alphaF()));
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


    QVector<QVector2D> texCoords = {
        QVector2D(0, 1),
        QVector2D(1, 1),
        QVector2D(0, 0),

        QVector2D(1, 1),
        QVector2D(0, 0),
        QVector2D(1, 0),
    };

    if (current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_textured_quad) {
        for (int i=6; i > 0; i-- ) {
            // TODO: Don't add/draw the non-textured polygon if we're going to be showing it textured.
            this->screen_vertices_textured.vertices.append(this->vertices.at(this->vertices.size()-i)); // Note: This duplicates vertices, so coloured poly drawn first, then textured.
            this->screen_vertices_textured.colors.append(QVector4D(1.0, 0, 0, 1.0));
            this->screen_vertices_textured.texCoords.append(texCoords[6-i]);
        }



        auto word = current_command->parameters[2].toUInt();

        auto clut = byte_of_quint32(word, 0) << 8 | byte_of_quint32(word, 1);

        auto clut_xcoord = (clut & 0b111111) * 16;
        auto clut_ycoord = ((clut >> 6) & 0b111111111);

        qDebug() << "clut_xcoord:" << clut_xcoord;
        qDebug() << "clut_ycoord:"<< clut_ycoord;


        word = current_command->parameters[4].toUInt();

        auto texpage = byte_of_quint32(word, 0) << 8 | byte_of_quint32(word, 1);

        auto texpage_xbase = (texpage & 0b1111) * 64;
        auto texpage_ybase = ((texpage >> 4) & 0b1) * 256;

        qDebug() << "texpage_xbase:" << texpage_xbase;
        qDebug() << "texpage_ybase:" << texpage_ybase;


//        this->point_to_texture_lookup.value(std::make_pair(texpage_xbase, texpage_ybase));

        auto found_texture = this->point_to_texture_lookup.value((texpage_xbase << 16 | texpage_ybase));

        // TODO: Handle texture not found?
        this->screen_vertices_textured.textures.append(found_texture);

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


    this->textures_loaded.append(new QOpenGLTexture(current_command->texture_raw.mirrored()));

    this->textures_loaded.last()->setMagnificationFilter(QOpenGLTexture::Nearest);

    //        this->point_to_texture_lookup[std::make_pair(position_vertex.x(), position_vertex.y())] = this->textures.last();
    this->point_to_texture_lookup[(position_vertex.x() << 16 | position_vertex.y())] = this->textures_loaded.last();

    this->point_to_texture_command_lookup[(position_vertex.x() << 16 | position_vertex.y())] = current_command;

    this->drawTexture(current_command);

}


void GLRenderer::doRender() {
    update();
}


void GLRenderer::clear() {
    this->resetRequested = true;
}
