#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <QByteArray>
#include <QVariantMap>
#include <QJsonDocument>

class ISerializer
{
public:
    virtual QByteArray Process(const QVariantMap& data) = 0;
    virtual ~ISerializer() {}
};

class IDeserializer
{
public:
    virtual QList<QVariantMap> Process(const QByteArray& data) = 0;
    virtual QVariantMap ProcessOne(const QByteArray& data) = 0;
    virtual ~IDeserializer() {}
};

class JsonSerializer : public ISerializer
{
public:
    QByteArray Process(const QVariantMap& data) override;
    ~JsonSerializer(){}
};

class JsonDeserializer : public IDeserializer
{
public:
    QList<QVariantMap> Process(const QByteArray& data) override;
    QVariantMap ProcessOne(const QByteArray& data) override;
    ~JsonDeserializer(){}
};

#endif // SERIALIZATION_H
