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
