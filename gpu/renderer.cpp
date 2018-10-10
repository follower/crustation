#include "command.h"
#include "renderer.h"

#include <QPainter>


Renderer::Renderer(QLabel *displayWidget)
    : display(displayWidget),
      Vram(QSize(1024, 512), QImage::Format_RGB888) { // TODO: Figure out most appropriate format to use...

}


void Renderer::initVram() {
    this->Vram.fill(Qt::gray);
    this->doRender();
}


void Renderer::requestRender() {
    this->doRender();
}


void Renderer::doRender() {
    this->display->setPixmap(QPixmap::fromImage(this->Vram));
}


void Renderer::drawPolygon(GpuCommand *current_command, bool useItemColor) {

    // Note: We recreate the QPainter instance each time because the docs
    //       seem to suggest they're only intended to be short-lived.
    QPainter painter(&this->Vram);

    QList<QPoint> points;
    for (QVariant item: current_command->parameters) {
        if (item.canConvert<QPoint>()) {
            points.append(item.toPoint());
        }
    }

    if (current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_monochrome_quad
            || current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_shaded_quad
            || current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_textured_quad) {
        points.swap(points.size()-1, points.size()-2);
    }


    if (useItemColor) {
        // TODO: Handle multiple colours correctly.
        painter.setBrush(QBrush(QColor(current_command->parameters.first().value<QColor>())));
    } else {
        //painter.setCompositionMode(); // TODO: Use XOR instead?
        //painter.setBrush(QBrush(Qt::green));
        //painter.setBrush(QBrush(QColor(current_command->parameters.first().value<QColor>())));
        painter.setPen(Qt::green);
    }

    painter.drawPolygon(QPolygon::fromList(points));
}
