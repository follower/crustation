#ifndef RENDERER_H
#define RENDERER_H

#include <QLabel>

#include "command.h"


class Renderer {

public:
    Renderer(QLabel *displayWidget);

    void initVram();
    void requestRender();

    void drawPolygon(GpuCommand *current_command, bool useItemColor = true);

private:
    QLabel *display;
    QImage Vram;

    void doRender();
};

#endif // RENDERER_H
