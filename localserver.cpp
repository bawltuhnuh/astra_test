#include "localserver.h"

LocalServer::LocalServer(TextEdit &text_edit, const QString &name, QObject *parent) :
    QObject(parent),
    m_textEdit(text_edit),
    m_name(name.isEmpty() ? "default" : name)
{
    connect(&m_server, &QLocalServer::newConnection, this, &LocalServer::newConnection);
    if (m_server.listen(m_name))
    {
        m_serverMode = true;
        if (!text_edit.load(m_name))
        {
            text_edit.fileNew();
        }
    } else
    {
        qDebug() << m_server.errorString();
        connect(&m_socket, &QLocalSocket::errorOccurred, this, &LocalServer::socketError);
        m_socket.connectToServer(m_name);
        m_socket.waitForReadyRead(3000);
        QByteArray init_data = m_socket.readAll();
        m_textEdit.loadPlainData(init_data == "NONE" ? "" : init_data);
        connect(&m_socket, &QLocalSocket::readyRead, this, &LocalServer::readyRead);
    }
    connect(&m_textEdit, &TextEdit::documentChanged, this, &LocalServer::documentChanged);
    text_edit.show();
}

LocalServer::~LocalServer()
{
    m_server.close();
    if (!m_sockets.isEmpty())
    {
        passServerRole();
    }
}

void LocalServer::newConnection()
{
    while (m_server.hasPendingConnections())
    {
        QLocalSocket* socket = m_server.nextPendingConnection();
        connect(socket, &QLocalSocket::readyRead, this, &LocalServer::readyRead);
        connect(socket, &QLocalSocket::errorOccurred, this, &LocalServer::socketError);
        m_sockets.push_back(socket);
        sendBodyToNewbie();
    }
}

void LocalServer::readyRead()
{
    QLocalSocket* editing_socket = (QLocalSocket*) sender();
    QByteArray data = editing_socket->readAll();
    if (data == "RUN_SERVER")
    {
        while (!m_server.listen(m_name)) {}
    } else if (data == "SERVER_DOWN")
    {
        do
        {
            m_socket.connectToServer(m_name);
        } while (!m_socket.waitForConnected());
    } else
    {
        applyChangesToDocument(data);
        if (m_serverMode)
        {
            for (auto& socket : m_sockets)
            {
                if (socket != editing_socket)
                {
                    socket->write(data);
                }
            }
        }
    }
}

void LocalServer::socketError()
{
    qDebug() << __FUNCTION__ << m_socket.errorString() << " " << m_socket.state();
}

void LocalServer::passServerRole()
{
    QLocalSocket* socket = m_sockets.first();
    socket->write("RUN_SERVER");
    for (int i = 1; i < m_sockets.size(); ++i)
    {
        m_sockets[i]->write("SERVER_DOWN");
    }
}

void LocalServer::sendBodyToNewbie()
{
    QLocalSocket* socket = m_sockets.back();
    socket->write(m_textEdit.document()->isEmpty() ? "NONE" : m_textEdit.document()->toPlainText().toUtf8());
}

void LocalServer::applyChangesToDocument(const QByteArray &data)
{
    QVariantMap map = QJsonDocument::fromJson(data).toVariant().toMap();
    int position = map["position"].toInt();
    int removed = map["removed"].toInt();
    QString added = map["added"].toString();
    QTextCursor cursor(m_textEdit.document());
    cursor.setPosition(position);
    disconnect(&m_textEdit, &TextEdit::documentChanged, this, &LocalServer::documentChanged);
    for (int i = 0; i < removed; ++i)
    {
        cursor.deleteChar();
    }
    cursor.insertText(added == "none" ? "" : added);
    connect(&m_textEdit, &TextEdit::documentChanged, this, &LocalServer::documentChanged);
}

void LocalServer::documentChanged(int position, int charRemoved, int charAdded)
{
    QString added_chars;
    for (int i = position; i < position + charAdded; ++i)
    {
        added_chars.append(m_textEdit.document()->characterAt(i));
    }
    QVariantMap map;
    map["position"] = position;
    map["removed"] = charRemoved;
    map["added"] = added_chars.isEmpty() ? "none" : added_chars;
    QByteArray data = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    if (m_serverMode)
    {
        for (auto& socket : m_sockets)
        {
            socket->write(data);
        }
    } else
    {
        m_socket.write(data);
    }
}
