#include "serialization.h"

QByteArray JsonSerializer::Process(const QVariantMap &data)
{
    return QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact);
}


QList<QVariantMap> JsonDeserializer::Process(const QByteArray &data)
{
    QList<QVariantMap> result;
    QList<QByteArray> messages;
    int start_pos = 0;
    int end_pos = data.indexOf("}{");
    while (end_pos != -1)
    {
        messages.push_back(data.mid(start_pos, end_pos - start_pos + 1));
        start_pos = end_pos + 1;
        end_pos = data.indexOf("}{", start_pos);
    }
    messages.push_back(data.mid(start_pos));
    for (auto& message : messages)
    {
        result.push_back(ProcessOne(message));
    }
    return result;
}

QVariantMap JsonDeserializer::ProcessOne(const QByteArray &data)
{
    QJsonParseError parse_error;
    QVariantMap result = QJsonDocument::fromJson(data, &parse_error).toVariant().toMap();
    if (parse_error.error != QJsonParseError::NoError)
    {
        qDebug() << __FUNCTION__ << parse_error.errorString() << '\n' << data;
        return QVariantMap();
    }
    return result;
}
