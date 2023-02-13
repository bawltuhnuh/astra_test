#include "localserver.h"

#include <QTextDocumentFragment>
#include <QTextBlock>

const QString LocalServer::MessageField::TYPE = "type";
const QString LocalServer::MessageField::POSITION = "position";
const QString LocalServer::MessageField::REMOVED = "removed";
const QString LocalServer::MessageField::ADDED = "added";
const QString LocalServer::MessageField::VALUE = "value";
const QString LocalServer::MessageField::BOLD = "bold";
const QString LocalServer::MessageField::UNDERLINE = "underline";
const QString LocalServer::MessageField::ITALIC = "italic";
const QString LocalServer::MessageField::FAMILY = "family";
const QString LocalServer::MessageField::SIZE = "size";
const QString LocalServer::MessageField::COLOR = "color";

const QString LocalServer::MessageValue::NONE = "none";

LocalServer::LocalServer(TextEdit& text_edit, const QString& name, ISerializer* serializer, IDeserializer* deserializer, QObject* parent)  :
    QObject(parent),
    m_textEdit(text_edit),
    m_name(name.isEmpty() ? "default" : name),
    m_serializer(serializer),
    m_deserializer(deserializer)
{
    connect(&m_server, &QLocalServer::newConnection, this, &LocalServer::newConnection);
    if (m_server.listen(m_name))
    {
        m_serverMode = true;
        connect(&m_textEdit, &TextEdit::contentsChange, this, &LocalServer::contentsChange);
        connect(&m_textEdit, &TextEdit::styleChanged, this, &LocalServer::styleChanged);
    } else
    {
        qDebug() << m_server.errorString();
        connect(&m_socket, &QLocalSocket::errorOccurred, this, &LocalServer::socketError);
        connect(&m_socket, &QLocalSocket::readyRead, this, &LocalServer::readyRead);
        m_socket.connectToServer(m_name);
    }
}

LocalServer::~LocalServer()
{
    disconnect(&m_textEdit, &TextEdit::contentsChange, this, &LocalServer::contentsChange);
    disconnect(&m_textEdit, &TextEdit::styleChanged, this, &LocalServer::styleChanged);
    if (m_serverMode)
    {
        m_server.close();
        if (!m_sockets.isEmpty())
        {
            passServerRole();
        }
    }
}

void LocalServer::newConnection()
{
    while (m_server.hasPendingConnections())
    {
        QLocalSocket* socket = m_server.nextPendingConnection();
        connect(socket, &QLocalSocket::readyRead, this, &LocalServer::readyRead);
        connect(socket, &QLocalSocket::errorOccurred, this, &LocalServer::socketError);
        connect(socket, &QLocalSocket::disconnected, this, &LocalServer::disconnectFromServer);
        m_sockets.push_back(socket);
        sendBodyToNewbie();
    }
}

void LocalServer::readyRead()
{
    QLocalSocket* editing_socket = (QLocalSocket*) sender();
    QByteArray data = editing_socket->readAll();
    handleMessage(editing_socket, data);
}

void LocalServer::socketError()
{
    QLocalSocket* socket = (QLocalSocket*) sender();
    qDebug() << __FUNCTION__ << socket->errorString() << " " << socket->state();
}

void LocalServer::disconnectFromServer()
{
    QLocalSocket* sender_socket = (QLocalSocket*) sender();
    for (auto& socket : m_sockets)
    {
        if (sender_socket == socket)
        {
            m_sockets.removeOne(sender_socket);
            sender_socket->deleteLater();
        }
    }
}

void LocalServer::styleChanged(int style_index, int position)
{
    QVariantMap map;
    map[MessageField::TYPE] = kStyleChanged;
    map[MessageField::POSITION] = position;
    map[MessageField::VALUE] = style_index;
    sendData(m_serializer->Process(map));
}

void LocalServer::handleMessage(QLocalSocket* editing_socket, const QByteArray &message)
{
    QList<QVariantMap> maps = m_deserializer->Process(message);
    if (maps.isEmpty())
    {
        return;
    }
    for (auto& map : maps)
    {
        int type = map[MessageField::TYPE].toInt();
        switch (type)
        {
            case MessageType::kInit:
            {
                handleInitMessage(map[MessageField::VALUE].toString());
                break;
            }
            case MessageType::kContentChangedWithHtml:
            {
                changeContentWithHtml(map);
                if (m_serverMode)
                {
                    for (auto& socket : m_sockets)
                    {
                        if (socket != editing_socket)
                        {
                            socket->write(message);
                        }
                    }
                }
                break;
            }
            case MessageType::kRunServer:
            {
                handleRunServerMessage();
                break;
            }
            case MessageType::kServerDown:
            {
                handleServerDownMessage();
                break;
            }
            case MessageType::kStyleChanged:
            {
                changeContentStyle(map);
                break;
            }
        }
    }
}

void LocalServer::handleInitMessage(const QString &init_data)
{
    m_textEdit.loadExternalData(init_data == MessageValue::NONE ? QString() : init_data);
    connect(&m_textEdit, &TextEdit::contentsChange, this, &LocalServer::contentsChange);
    connect(&m_textEdit, &TextEdit::styleChanged, this, &LocalServer::styleChanged);
}

void LocalServer::handleRunServerMessage()
{
    while (!m_server.listen(m_name)) {}
    m_serverMode = true;
}

void LocalServer::handleServerDownMessage()
{
    if (m_socket.state() == QLocalSocket::ConnectedState)
    {
        m_socket.disconnectFromServer();
    }
    do
    {
        m_socket.connectToServer(m_name);
    } while (!m_socket.waitForConnected());
}

void LocalServer::changeContentStyle(const QVariantMap &map)
{
    disconnect(&m_textEdit, &TextEdit::styleChanged, this, &LocalServer::styleChanged);
    QTextCursor cursor(m_textEdit.document());
    cursor.setPosition(map[MessageField::POSITION].toInt());
    m_textEdit.externalSetTextStyleByIndex(map[MessageField::VALUE].toInt());
    connect(&m_textEdit, &TextEdit::styleChanged, this, &LocalServer::styleChanged);
}

void LocalServer::passServerRole()
{
    QLocalSocket* socket = m_sockets.first();

    QVariantMap map;
    map[MessageField::TYPE] = kRunServer;

    socket->write(m_serializer->Process(map));
    socket->flush();

    map[MessageField::TYPE] = kServerDown;

    QByteArray down_message = m_serializer->Process(map);

    for (int i = 1; i < m_sockets.size(); ++i)
    {
        m_sockets[i]->write(down_message);
        m_sockets[i]->flush();
    }
    for (auto& socket : m_sockets)
    {
        delete socket;
    }
}

void LocalServer::sendBodyToNewbie()
{
    QLocalSocket* socket = m_sockets.back();
    QVariantMap map;
    map[MessageField::TYPE] = kInit;
    disconnect(&m_textEdit, &TextEdit::contentsChange, this, &LocalServer::contentsChange);
    map[MessageField::VALUE] = m_textEdit.document()->isEmpty() ? MessageValue::NONE : m_textEdit.document()->toHtml();
    connect(&m_textEdit, &TextEdit::contentsChange, this, &LocalServer::contentsChange);
    socket->write(m_serializer->Process(map));
}

void LocalServer::changeContentWithHtml(const QVariantMap& map)
{
    int position = map[MessageField::POSITION].toInt();
    int removed = map[MessageField::REMOVED].toInt();
    QString added = map[MessageField::ADDED].toString();

    disconnect(&m_textEdit, &TextEdit::contentsChange, this, &LocalServer::contentsChange);

    QTextCursor cursor(m_textEdit.document());

    cursor.beginEditBlock();
    cursor.setPosition(position);

    int style = m_textEdit.getStyle();

    for (int i = 0; i < removed; ++i)
    {
        cursor.deleteChar();
    }

    if (added != MessageValue::NONE)
    {
        cursor.insertFragment(QTextDocumentFragment::fromHtml(added));
    }

    cursor.endEditBlock();

    connect(&m_textEdit, &TextEdit::contentsChange, this, &LocalServer::contentsChange);
}

void LocalServer::sendData(const QByteArray &data)
{
    if (m_serverMode)
    {
        for (auto& socket : m_sockets)
        {
            socket->write(data);
            socket->flush();
        }
    } else
    {
        m_socket.write(data);
        m_socket.flush();
    }
}

void LocalServer::contentsChange(int position, int charRemoved, int charAdded)
{
    QString added_text = MessageValue::NONE;

    disconnect(&m_textEdit, &TextEdit::contentsChange, this, &LocalServer::contentsChange);
    if (charAdded)
    {
        QTextCursor cursor(m_textEdit.document());
        cursor.setPosition(position);
        int style = m_textEdit.getStyle();
        cursor.setPosition(position + charAdded, QTextCursor::KeepAnchor);
        added_text = cursor.selection().toHtml();
    }

    if (added_text.isEmpty())
    {
        connect(&m_textEdit, &TextEdit::contentsChange, this, &LocalServer::contentsChange);
        return;
    }

    connect(&m_textEdit, &TextEdit::contentsChange, this, &LocalServer::contentsChange);
    QVariantMap map;
    map[MessageField::TYPE] = kContentChangedWithHtml;
    map[MessageField::POSITION] = position;
    map[MessageField::REMOVED] = charRemoved;
    map[MessageField::ADDED] = added_text;
    sendData(m_serializer->Process(map));
}
