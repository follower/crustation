#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QStandardItemModel>
#include <QVector2D>
#include <QPainter>
#include <QMetaEnum>
#include <QBuffer>
#include <QFileDialog>

QStandardItemModel model;

QImage image2(QSize(1024, 512), QImage::Format_RGB888); // TODO: Figure out most appropriate format to use...


// TODO: Do this better/more robust...
// Note: MSB[byte0][byte1][byte2][byte3]LSB
#define byte_of_quint32(the_value, byte_index) ((the_value >> ((3-byte_index)*8)) & 0xff)

QColor colorFromWord(quint32 word) {

    QColor color(qRgb(byte_of_quint32(word, 3), byte_of_quint32(word, 2), byte_of_quint32(word, 1)));

    qDebug("R: 0x%02x G: 0x%02x B: 0x%02x", color.red(), color.green(), color.blue());

    return color;
}


QPoint pointFromWord(quint32 word) {

    int y = byte_of_quint32(word, 0) << 8 | byte_of_quint32(word, 1);
    int x = byte_of_quint32(word, 2) << 8 | byte_of_quint32(word, 3);

    QPoint vertex(x, y);

    qDebug("x: %4d y: %4d", x, y);
    qDebug() << "vertex: " << vertex;

    return vertex;
}


class GpuCommand : public QStandardItem {

    Q_GADGET

public:

    using QStandardItem::QStandardItem;

    enum Gpu0_Opcodes {
        // Note: Names from Rustation code.
        // TODO: Change approach to match that used by Rustation which doesn't name the individual opcodes?
        gp0_nop = 0x00,
        gp0_clear_cache = 0x01,
        gp0_fill_rect = 0x02,
        gp0_monochrome_triangle = 0x20,
//        gp0_monochrome_triangle = 0x22,
        gp0_textured_triangle = 0x24,
//        gp0_textured_triangle = 0x25,
//        gp0_textured_triangle = 0x26,
//        gp0_textured_triangle = 0x27,
        gp0_monochrome_quad = 0x28,
//        gp0_monochrome_quad = 0x2a,
        gp0_textured_quad = 0x2c,
//        gp0_textured_quad = 0x2d,
//        gp0_textured_quad = 0x2e,
//        gp0_textured_quad = 0x2f,
        gp0_shaded_triangle = 0x30,
//        gp0_shaded_triangle = 0x32,
        gp0_textured_shaded_triangle = 0x34,
//        gp0_textured_shaded_triangle = 0x36,
        gp0_shaded_quad = 0x38,
//        gp0_shaded_quad = 0x3a,
        gp0_textured_shaded_quad = 0x3c,
//        gp0_textured_shaded_quad = 0x3e,
        gp0_monochrome_line = 0x40,
//        gp0_monochrome_line = 0x42,
        gp0_monochrome_polyline = 0x48,
//        gp0_monochrome_polyline = 0x4a,
        gp0_shaded_line = 0x50,
//        gp0_shaded_line = 0x52,
        gp0_shaded_polyline = 0x58,
//        gp0_shaded_polyline = 0x5a,
        gp0_monochrome_rect = 0x60,
//        gp0_monochrome_rect = 0x62,
        gp0_textured_rect = 0x64,
//        gp0_textured_rect = 0x65,
//        gp0_textured_rect = 0x66,
//        gp0_textured_rect = 0x67,
        gp0_monochrome_rect_1x1 = 0x68,
//        gp0_monochrome_rect_1x1 = 0x6a,
        gp0_textured_rect_8x8 = 0x74,
//        gp0_textured_rect_8x8 = 0x75,
//        gp0_textured_rect_8x8 = 0x76,
//        gp0_textured_rect_8x8 = 0x77,
        gp0_monochrome_rect_16x16 = 0x78,
//        gp0_monochrome_rect_16x16 = 0x7a,
        gp0_textured_rect_16x16 = 0x7c,
//        gp0_textured_rect_16x16 = 0x7d,
//        gp0_textured_rect_16x16 = 0x7e,
//        gp0_textured_rect_16x16 = 0x7f,
        gp0_copy_rect = 0x80,
        gp0_image_load = 0xa0,
        gp0_image_store = 0xc0,
        gp0_draw_mode = 0xe1,
        gp0_texture_window = 0xe2,
        gp0_drawing_area_top_left = 0xe3,
        gp0_drawing_area_bottom_right = 0xe4,
        gp0_drawing_offset = 0xe5,
        gp0_mask_bit_setting = 0xe6,
    };

    Q_ENUM(Gpu0_Opcodes) // Enables enum value to name lookup.

    enum Gpu1_Opcodes {
        // Note: Names from Rustation code.
        gp1_reset = 0x00,
        gp1_reset_command_buffer = 0x01,
        gp1_acknowledge_irq = 0x02,
        gp1_display_enable = 0x03,
        gp1_dma_direction = 0x04,
        gp1_display_vram_start = 0x05,
        gp1_display_horizontal_range = 0x06,
        gp1_display_vertical_range = 0x07,
        gp1_display_mode = 0x08,
        gp1_get_info = 0x10
    };

    Q_ENUM(Gpu1_Opcodes) // Enables enum value to name lookup.

    int targetGpu = -1;
    int command_value = -1;

    QList<QVariant> parameters;

    QBuffer data; // Associated (DMA) transferred data. (Primarily for 'gp0_image_load' (0xa0) command.)


    static GpuCommand *fromFields(QString targetGpu, quint32 command) {
        auto result = new GpuCommand(QString::number(command, 16));
        result->targetGpu = targetGpu.at(targetGpu.size()-1).digitValue();
        result->command_value = (command >> 24) & 0xff;

        QString command_name("<unknown target gpu>");

        if (result->targetGpu == 0) {
            command_name = QMetaEnum::fromType<GpuCommand::Gpu0_Opcodes>().valueToKey(result->command_value);
        } else if (result->targetGpu == 1) {
            command_name = QMetaEnum::fromType<GpuCommand::Gpu1_Opcodes>().valueToKey(result->command_value);
        }

        result->setText(QString("%1 (0x%2)").arg(command_name).arg(result->command_value, 2, 16, QChar('0')));

        return result;
    }


    // TODO: Better separate the model/view aspects...

    QColor addColorParameter(quint32 parameter_word, bool decorate_parent = false) {
        auto color = colorFromWord(parameter_word);
        this->parameters.append(color);

        auto this_item = new QStandardItem(color.name());
        this_item->setData(color, Qt::DecorationRole);
        this->appendRow(this_item);

        if (decorate_parent) {
            // TODO: Handle display of multiple colors. (Decoration only intended for mono commands currently.)
            this->setData(color, Qt::DecorationRole);
        }

        return color;
    }


    QPoint addVertexParameter(quint32 parameter_word) {
        auto vertex = pointFromWord(parameter_word);

        this->parameters.append(vertex);
        this->appendRow(new QStandardItem(QString("(%1, %2)").arg(vertex.x(), 4).arg(vertex.y(), 4)));

        return vertex;
    }
};


void MainWindow::command_onCurrentChanged(const QModelIndex &current, const QModelIndex &previous) {
    qDebug() << "current changed" << current << previous;

    if (current.isValid()) {
        auto current_command = static_cast<GpuCommand *>(model.itemFromIndex(current));

        // TODO: Do this properly/replay/etc...
        if (current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_shaded_triangle
                || current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_shaded_quad
                || current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_monochrome_quad
                || current_command->command_value == GpuCommand::Gpu0_Opcodes::gp0_textured_quad) {
            QPainter painter(&image2);
            drawPolygon(painter, current_command, false);
            ui->label_2->setPixmap(QPixmap::fromImage(image2));
        }
    }
}


void MainWindow::drawPolygon(QPainter &painter, GpuCommand *current_command, bool useItemColor = true)
{
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


void MainWindow::initUi() {

    image2.fill(Qt::gray);
    ui->label_2->setPixmap(QPixmap::fromImage(image2));
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

    initUi();
}


void MainWindow::loadFile(QString logFilePath) {

    model.setRowCount(0);

    initUi();


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

    //QImage image2(QSize(1024, 512), QImage::Format_RGB888); // TODO: Figure out most appropriate format to use...
    QPainter painter(&image2);
    painter.setPen(Qt::red);
    ui->label_2->setPixmap(QPixmap::fromImage(image2));

    while(!input_log.atEnd()) {

        input_log >> current_field;
        qDebug() << "current_field: " << current_field;

        if (current_field=="") { // TODO: Get `.atEnd()` check to work properly...
            break;
        }


        input_log >> current_numeric_field;
        qDebug("current_numeric_field: 0x%x", current_numeric_field);


        qDebug() << "remaining: " << lines_remaining_in_this_command;

        if (current_field == "GP1") { // TODO: Do this with less duplication...
            current_gpu1_command = GpuCommand::fromFields(current_field, current_numeric_field);

            auto item_raw = new QStandardItem(QString("0x%1").arg(current_numeric_field, 8, 16, QChar('0')));
            parentItem->appendRow({current_gpu1_command, item_raw}); // TODO: Do this properly?

            line_count++;

            current_gpu1_command = nullptr;

            continue;
        }

        if (lines_remaining_in_this_command == 0) {

            current_command = GpuCommand::fromFields(current_field, current_numeric_field);
            auto item_raw = new QStandardItem(QString("0x%1").arg(current_numeric_field, 8, 16, QChar('0')));
            parentItem->appendRow({current_command, item_raw}); // TODO: Do this properly?

            qDebug("\n----------");
            qDebug() << "gpu: " << current_command->targetGpu;
            qDebug("command_value: 0x%x", current_command->command_value);
            qDebug() << QMetaEnum::fromType<GpuCommand::Gpu0_Opcodes>().valueToKey(current_command->command_value);

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
                qDebug() << "Target GPU not recognized!";
                abort();
            }

        }

        qDebug() << "current_command: " << current_command;
        qDebug() << "linecount: " << line_count;



        if (current_command!=nullptr) {

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
                        drawPolygon(painter, current_command);
                    }

                    ui->label_2->setPixmap(QPixmap::fromImage(image2));

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
                    if (lines_remaining_in_this_command==1) {
                        drawPolygon(painter, current_command);
                    }

                    ui->label_2->setPixmap(QPixmap::fromImage(image2));
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
                        drawPolygon(painter, current_command);
                    }

                    ui->label_2->setPixmap(QPixmap::fromImage(image2));

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
#endif
                        ui->label_2->setPixmap(QPixmap::fromImage(image2));
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

    this->loadFile(fileName);

    QApplication::restoreOverrideCursor();
#endif
    ui->treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

// TODO: Remove this when GpuCommand::Opcodes is moved into .h file.
#include "mainwindow.moc"
