#include "mainwindow.h"
#include "ui_mainwindow.h"

Q_LOGGING_CATEGORY(crustFileLoad, "crust.file.load", QtInfoMsg /*QtDebugMsg*/)

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QStandardItemModel>
#include <QVector2D>
#include <QBuffer>
#include <QFileDialog>
#include <QTimer>


QStandardItemModel model;

unsigned int playbackDelay_ms = 1000;


void MainWindow::command_onCurrentChanged(const QModelIndex &current, const QModelIndex &previous) {

    bool renderRequired = false;

    if (current.isValid()) {
        auto current_command = static_cast<GpuCommand *>(model.itemFromIndex(current));

        // TODO: Do this properly/replay/etc...
        if (current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_shaded_triangle
                || current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_shaded_quad
                || current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_monochrome_quad
                || current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_textured_quad) {
            this->renderer->drawPolygon(current_command, this->isPlaying);
            renderRequired = !this->isPlaying || (this->isPlaying && playbackDelay_ms>100); // TODO: Allow "smart" render-required detection to be user selectable?

            this->glRenderer->drawPolygon(current_command);
        }

        renderRequired = renderRequired || (this->isPlaying && (current_command->command_value == GpuCommand::Gpu1_Opcodes::gp1_display_vram_start));

        if (renderRequired) {
            this->renderer->requestRender();
            this->glRenderer->doRender();
            this->glRenderer->clear(); // ~
        }
    }
}


void MainWindow::setupMoreUi() {

    QActionGroup *menuActionGroup = new QActionGroup(this);

    ui->selectPlayback1second->setActionGroup(menuActionGroup);
    ui->selectPlayback200ms->setActionGroup(menuActionGroup);
    ui->selectPlayback10ms->setActionGroup(menuActionGroup);

    ui->actionPlay->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
    ui->actionPause->setIcon(this->style()->standardIcon(QStyle::SP_MediaPause));
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    model.setHorizontalHeaderLabels(QStringList({"GPU Command", "Raw"}));

    ui->treeView->setModel(&model);
    QObject::connect(ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged,
                     this, &MainWindow::command_onCurrentChanged);

    ui->treeView->header()->setSectionResizeMode(QHeaderView::Stretch);

    setupMoreUi();

    this->renderer = new Renderer(ui->labelVramView);
    this->renderer->initVram();

    this->glRenderer = ui->openGLWidget;
}


void MainWindow::loadFile(QString logFilePath) {

    model.setRowCount(0);

    this->renderer->initVram();


    QStandardItem *parentItem = model.invisibleRootItem();

    QFile input_file(logFilePath);
    input_file.open(QFile::ReadOnly | QFile::Text);

    QTextStream input_log(&input_file);

    QString current_field;

    quint32 current_numeric_field;

    int line_count = 0;

    // -----------

    int lines_remaining_in_this_command = 0;
    bool in_data_transfer = false;

    GpuCommand *current_command = nullptr;
    GpuCommand *current_gpu1_command = nullptr; // These can happen in the middle of GPU0 commands so need to be handled separately.

//    painter.setPen(Qt::red);
    this->renderer->requestRender();

    while(!input_log.atEnd()) {

        input_log >> current_field;

        qCDebug(crustFileLoad) << "current_field: " << current_field;

        if (current_field=="") { // TODO: Get `.atEnd()` check to work properly...
            break;
        }


        input_log >> current_numeric_field;
        qCDebug(crustFileLoad, "current_numeric_field: 0x%x", current_numeric_field);

        if (current_field == "GP1") { // TODO: Do this with less duplication...
            current_gpu1_command = GpuCommand::fromFields(current_field, current_numeric_field);
            current_gpu1_command->setToolTip(QString("Line: %1").arg(line_count));

            auto item_raw = new QStandardItem(QString("0x%1").arg(current_numeric_field, 8, 16, QChar('0')));
            parentItem->appendRow({current_gpu1_command, item_raw}); // TODO: Do this properly?

            line_count++;

            current_gpu1_command = nullptr;

            continue;
        }

        qCDebug(crustFileLoad) << "lines remaining: " << lines_remaining_in_this_command;

        if (lines_remaining_in_this_command == 0) {

            current_command = GpuCommand::fromFields(current_field, current_numeric_field);
            current_command->setToolTip(QString("Line: %1").arg(line_count));
            auto item_raw = new QStandardItem(QString("0x%1").arg(current_numeric_field, 8, 16, QChar('0')));
            parentItem->appendRow({current_command, item_raw}); // TODO: Do this properly?

            current_command->raw_lines = new QStandardItem("Full Raw");

            qCDebug(crustFileLoad) << "";
            qCDebug(crustFileLoad) << "----------";
            qCDebug(crustFileLoad) << "gpu: " << current_command->targetGpu;
            qCDebug(crustFileLoad, "command_value: 0x%x", current_command->command_value);
            qCDebug(crustFileLoad) << "command: " << QMetaEnum::fromType<GpuCommand::Gpu0_Opcodes>().valueToKey(current_command->command_value);

            if (current_command->targetGpu == 0) {

                switch(current_command->command_value) {
                case GpuCommand::Gpu0_Opcodes::gp0_nop:
                case GpuCommand::Gpu0_Opcodes::gp0_clear_cache:
                case GpuCommand::Gpu0_Opcodes::gp0_draw_mode:
                case GpuCommand::Gpu0_Opcodes::gp0_texture_window:
                case GpuCommand::Gpu0_Opcodes::gp0_drawing_area_top_left:
                case GpuCommand::Gpu0_Opcodes::gp0_drawing_area_bottom_right:
                case GpuCommand::Gpu0_Opcodes::gp0_drawing_offset:
                case GpuCommand::Gpu0_Opcodes::gp0_mask_bit_setting:
                    lines_remaining_in_this_command = 1;
                    break;

                case GpuCommand::Gpu0_Opcodes::gp0_image_load: // TODO: Verify data transfer handling...
                case GpuCommand::Gpu0_Opcodes::gp0_image_store: // TODO: Verify ignoring the data part of this okay...
                    lines_remaining_in_this_command = 3;
                    break;

                case GpuCommand::Gpu0_Opcodes::gp0_monochrome_quad:
                    lines_remaining_in_this_command = 5;
                    break;

                case GpuCommand::Gpu0_Opcodes::gp0_textured_quad:
                    lines_remaining_in_this_command = 9;
                    break;

                case GpuCommand::Gpu0_Opcodes::gp0_shaded_triangle:
                    lines_remaining_in_this_command = 6;
                    break;

                case GpuCommand::Gpu0_Opcodes::gp0_shaded_quad:
                    lines_remaining_in_this_command = 8;
                    break;

                default:
                    qWarning("Unhandled command: 0x%02x in 0x%08x", current_command->command_value, current_numeric_field);
                    line_count = 999999; // TODO: Handle this properly.
                    break;
                };

            } else if (current_command->targetGpu == 1) {
                // TODO: Handle this better?
            } else {
                qFatal("Target GPU not recognized!");
                abort();
            }

        }

        qCDebug(crustFileLoad) << "linecount: " << line_count;

        if (current_command!=nullptr) {

            // TODO: Handle this in a more tidy manner?
            current_command->raw_lines->appendRow({new QStandardItem(QString("%1").arg(current_command->raw_lines->rowCount()+1)),
                                                   new QStandardItem(QString("%1 %2 %3 %4").arg(byte_of_quint32(current_numeric_field, 0), 2, 16, QChar('0'))
                                                   .arg(byte_of_quint32(current_numeric_field, 1), 2, 16, QChar('0'))
                                                   .arg(byte_of_quint32(current_numeric_field, 2), 2, 16, QChar('0'))
                                                   .arg(byte_of_quint32(current_numeric_field, 3), 2, 16, QChar('0')))});

            switch(current_command->command_value) {
            case GpuCommand::Gpu0_Opcodes::gp0_monochrome_quad:
                switch(lines_remaining_in_this_command) {

                case 5:
                    current_command->addColorParameter(current_numeric_field, true);
                    break;

                case 4:
                case 3:
                case 2:
                case 1:
                    current_command->addVertexParameter(current_numeric_field);

                    if (lines_remaining_in_this_command==1) {
                        this->renderer->drawPolygon(current_command);
                        this->glRenderer->drawPolygon(current_command);
                    }

#define DRAW_IN_LOAD_FILE 1

#if DRAW_IN_LOAD_FILE
                    this->renderer->requestRender();
                    this->glRenderer->doRender(); //
#endif
                    break;

                default:
                    break;
                };
                break;


            case GpuCommand::Gpu0_Opcodes::gp0_textured_quad:
                switch(lines_remaining_in_this_command) {

                case 9:
                    current_command->addColorParameter(current_numeric_field);
                    break;

                case 8:
                case 6:
                case 4:
                case 2:
                    current_command->addVertexParameter(current_numeric_field);
                    break;

                // TODO: Handle these properly... (Texcoords, Palette CLUT, Texpage)
                case 7:
                case 5:
                case 3:
                case 1:
                    current_command->addOpaqueParameter(current_numeric_field);

                    if (lines_remaining_in_this_command==1) {
                        this->renderer->drawPolygon(current_command);
                        this->glRenderer->drawPolygon(current_command);
                    }

#if DRAW_IN_LOAD_FILE
                    this->renderer->requestRender();
                    this->glRenderer->doRender(); //
#endif
                    break;


                default:
                    break;
                };
                break;


            case GpuCommand::Gpu0_Opcodes::gp0_shaded_triangle:
            case GpuCommand::Gpu0_Opcodes::gp0_shaded_quad:
                switch(lines_remaining_in_this_command) {

                case 8: // quad only
                case 6:
                case 4:
                case 2:
                    current_command->addColorParameter(current_numeric_field);
                    break;

                case 7: // quad only
                case 5:
                case 3:
                case 1:
                    current_command->addVertexParameter(current_numeric_field);

                    if (lines_remaining_in_this_command==1) {
                        this->renderer->drawPolygon(current_command);
                        this->glRenderer->drawPolygon(current_command);
                    }

#if DRAW_IN_LOAD_FILE
                    this->renderer->requestRender();
                    this->glRenderer->doRender(); //
#endif

                    break;

                default:
                    break;
                }
                break;


            case GpuCommand::Gpu0_Opcodes::gp0_image_load:
                if (in_data_transfer) {

                    // 24-bit color...
                    // Bit: 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
                    //      00 BB BB BB BB BB GG GG GG GG GG RR RR RR RR RR

                    // Note: Not entirely sure about where this (correct) order is determined.
                    current_command->data.putChar(byte_of_quint32(current_numeric_field, 2));
                    current_command->data.putChar(byte_of_quint32(current_numeric_field, 3));
                    current_command->data.putChar(byte_of_quint32(current_numeric_field, 0));
                    current_command->data.putChar(byte_of_quint32(current_numeric_field, 1));


                    if (lines_remaining_in_this_command == 1) {

                        auto size_vertex = current_command->parameters.last().toPoint();

                        QImage image((unsigned char *) current_command->data.data().constData() /* LOL */, size_vertex.x(), size_vertex.y(), QImage::Format_RGB555);

                        auto new_item = new QStandardItem("");
                        new_item->setData(QPixmap::fromImage(image), Qt::DecorationRole);
                        current_command->appendRow(new_item);

                        current_command->data.close();

                        current_command->texture = new QImage(image);

                        in_data_transfer = false;
                    }


                } else {
                    switch(lines_remaining_in_this_command) {

                    case 2:
                        {
                        // Position
                        auto pos_vertex = pointFromWord(current_numeric_field);
                        pos_vertex.rx() &= 0x3ff;
                        pos_vertex.ry() &= 0x1ff;

                        current_command->parameters.append(pos_vertex);
                        current_command->appendRow(new QStandardItem(QString("Pos: (%1, %2)").arg(pos_vertex.x(), 4).arg(pos_vertex.y(), 4)));

#if 0
                        painter.drawPoint(vertex);
#if DRAW_IN_LOAD_FILE
                        this->renderer->requestRender();
#endif
#endif
                        }
                        break;

                    case 1:
                        {
                        // Size
                        auto size_vertex = pointFromWord(current_numeric_field);
                        size_vertex.setX(((size_vertex.x()-1) & 0x3ff)+1);
                        size_vertex.setY(((size_vertex.y()-1) & 0x1ff)+1);

                        current_command->parameters.append(size_vertex);
                        current_command->appendRow(new QStandardItem(QString("Size: %1 x %2").arg(size_vertex.x()).arg(size_vertex.y())));

                        current_command->data.open(QBuffer::ReadWrite | QBuffer::Truncate);

                        in_data_transfer = true;
                        lines_remaining_in_this_command = (size_vertex.x() * size_vertex.y())/2;

                        lines_remaining_in_this_command++; // Kludge to handle "current" line properly

                        }
                        break;

                    default:
                        break;
                    }
                }
            };


            if ((lines_remaining_in_this_command==1) && (current_command->raw_lines->rowCount() > 1)) {
                // TODO: Limit the number of lines? (e.g. for load data command)
                current_command->appendRow(current_command->raw_lines);
            }


        }

        if (current_command->targetGpu == 0) {
            lines_remaining_in_this_command--; // TODO: [Done?] Ensure we handle mixed GPU0 & GPU1 lines correctly.
        }

        line_count++;

#if 1
        if ((line_count % 500) == 0) {
            QCoreApplication::instance()->processEvents(QEventLoop::AllEvents, 1);
        }
#endif

        if (line_count > 20000 /* TODO: 20000*/) {
            break;
        }
    }

#if !DRAW_IN_LOAD_FILE
    this->renderer->requestRender();
    this->glRenderer->doRender(); //
#endif

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    auto fileName = QFileDialog::getOpenFileName(this);

#if 0
    QtConcurrent::run(this, &MainWindow::loadFile, fileName);
#else
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QCoreApplication::instance()->processEvents(QEventLoop::AllEvents, 1);

    this->setWindowFilePath(fileName);
    this->loadFile(fileName);

    QApplication::restoreOverrideCursor();
#endif
    ui->treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->treeView->setCurrentIndex(model.index(0,0));
}

void MainWindow::playNextCommand() {

    if (!this->isPlaying) {
        return;
    }

    auto nextSibling = ui->treeView->currentIndex().sibling(ui->treeView->currentIndex().row()+1, 0);

    if (nextSibling.isValid()) {
        ui->treeView->setCurrentIndex(nextSibling);
        QTimer::singleShot(playbackDelay_ms, this, &MainWindow::playNextCommand);
    } else {
        this->isPlaying = false;
    }

}

void MainWindow::on_actionPlay_triggered() {
    this->isPlaying = true;
    this->playNextCommand();
}

void MainWindow::on_actionPause_triggered() {
    this->isPlaying = false;
    // TODO: Also stop timer?
}

void MainWindow::on_selectPlayback1second_triggered() {
    playbackDelay_ms = 1000;
}

void MainWindow::on_selectPlayback200ms_triggered() {
    playbackDelay_ms = 200;
}

void MainWindow::on_selectPlayback10ms_triggered() {
    playbackDelay_ms = 10;
}
