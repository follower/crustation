#ifndef COMMAND_H
#define COMMAND_H

#include <QBuffer>
#include <QMetaEnum>
#include <QStandardItemModel>


// TODO: Do this better/more robust...
// Note: MSB[byte0][byte1][byte2][byte3]LSB
#define byte_of_quint32(the_value, byte_index) ((the_value >> ((3-byte_index)*8)) & 0xff)

QPoint pointFromWord(quint32 word); // TODO: Remove from this header once file loading is pulled into here.


// Parsing VRAM data...
QVector<QRgb> convertClutToPalette(QByteArray clut_table_data);
QImage convert4BitTextureToIndexedImage(QByteArray texture_4bit_indexed_data, QSize texture_size);


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
    QHash<QString, QVariant> named_parameters;

    QBuffer data; // Associated (DMA) transferred data. (Primarily for 'gp0_image_load' (0xa0) command.)
    QImage *texture; // Content of 'gp0_image_load' (0xa0) command as an image.

    QStandardItem *raw_lines;

    static GpuCommand *fromFields(QString targetGpu, quint32 command);

    // TODO: Better separate the model/view aspects...
    QColor addColorParameter(quint32 parameter_word, bool decorate_parent = false);
    QPoint addVertexParameter(quint32 parameter_word);
    quint32 addOpaqueParameter(quint32 parameter_word);

    void addNamedPointParameter(QString parameter_name, QPoint point_parameter);
    void addTexturePreview(QImage texture);
};

#endif // COMMAND_H
