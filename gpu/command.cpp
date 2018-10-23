#include "command.h"


extern QColor colorFromWord(quint32 word) {

    QColor color(qRgb(byte_of_quint32(word, 3), byte_of_quint32(word, 2), byte_of_quint32(word, 1)));

////    qCDebug(crustFileLoad, "R: 0x%02x G: 0x%02x B: 0x%02x", color.red(), color.green(), color.blue());

    return color;
}


extern QPoint pointFromWord(quint32 word) {

    int y = byte_of_quint32(word, 0) << 8 | byte_of_quint32(word, 1);
    int x = byte_of_quint32(word, 2) << 8 | byte_of_quint32(word, 3);

    QPoint vertex(x, y);

////    qCDebug(crustFileLoad) << "vertex (x, y): " << vertex;

    return vertex;
}


// MSB[x][00000][11111][22222]LSB
#define five_bit_group_from_word(the_word, group_index) ((the_word >> ((2-group_index)*5)) & 0b11111)

QList<QColor> convertClutToPalette(QByteArray clut_table_data) {

    // Parses bunch of bytes from a VRAM transfer as Color Look-Up Table
    // and creates a palette in a form we can use more easily.

    QList<QColor> palette;

    // Note: Assumes it's a 4-bit palette/Color Look Up Table...
    // TODO: Include other size CLUTs?
    for (auto offset = 0; offset < clut_table_data.size(); offset+=2) {
        quint16 clut_entry = (static_cast<quint8>(clut_table_data.at(offset)) << 8) | static_cast<quint8>(clut_table_data.at(offset+1));

        // TODO: Handle the 5-bit color conversion properly...
        QColor this_color(five_bit_group_from_word(clut_entry, 2) * 8,
                          five_bit_group_from_word(clut_entry, 1) * 8,
                          five_bit_group_from_word(clut_entry, 0) * 8,
                          (clut_entry == 0x0000) ? 0 : 255);
        palette << this_color;
    }

    return palette;
}


GpuCommand *GpuCommand::fromFields(QString targetGpu, quint32 command) {
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


QColor GpuCommand::addColorParameter(quint32 parameter_word, bool decorate_parent) {
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


QPoint GpuCommand::addVertexParameter(quint32 parameter_word) {
    auto vertex = pointFromWord(parameter_word);

    this->parameters.append(vertex);
    this->appendRow(new QStandardItem(QString("(%1, %2)").arg(vertex.x(), 4).arg(vertex.y(), 4)));

    return vertex;
}


quint32 GpuCommand::addOpaqueParameter(quint32 parameter_word) {
    this->parameters.append(parameter_word);
    this->appendRow(new QStandardItem(QString("Param: 0x%1").arg(parameter_word, 8, 16, QChar('0'))));
    return parameter_word;
}
