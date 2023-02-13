#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#include "textedit.h"

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTextDocument>
#include "serialization.h"

#include <QJsonDocument>
#include <QTextCursor>
#include <QEventLoop>

class LocalServer : public QObject
{
    Q_OBJECT
public: 
    enum MessageType
    {
        kInit,
        kTextChanged,
        kRunServer,
        kServerDown,
        kStyleChanged
    };

    struct MessageField
    {
        static const QString TYPE;
        static const QString POSITION;
        static const QString REMOVED;
        static const QString ADDED;
        static const QString VALUE;
    };

    struct MessageValue
    {
        static const QString NONE;
    };

    LocalServer(TextEdit& text_edit, const QString& name, ISerializer* serializer, IDeserializer* deserializer, QObject* parent = nullptr);

    ~LocalServer();

private slots:
    void contentsChange(int position, int charRemoved, int charAdded);

    void newConnection();

    void readyRead();

    void socketError();

    void disconnectFromServer();

    void styleChanged(int style_index, int position);

private:
    void handleMessage(QLocalSocket* editing_socket, const QByteArray& message);

    void passServerRole();

    void sendBodyToNewbie();

    void applyTextChangesToDocument(const QVariantMap& data);

    void sendData(const QByteArray& data);

private:
    struct Change
    {
        int position;
        int charRemoved;
        int charAdded;
    };

    QLocalServer m_server;
    QLocalSocket m_socket;

    QList<QLocalSocket*> m_sockets;

    QList<Change> m_changes;

    TextEdit& m_textEdit;
    QString m_name;

    QScopedPointer<ISerializer> m_serializer;
    QScopedPointer<IDeserializer> m_deserializer;

    //MessageHandler m_handler;

    bool m_serverMode = false;
};

#endif // LOCALSERVER_H
