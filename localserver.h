#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#include "textedit.h"

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTextDocument>
#include <QJsonDocument>
#include <QTextCursor>

class LocalServer : public QObject
{
    Q_OBJECT
public:
    LocalServer(TextEdit& text_edit, const QString& name, QObject* parent = nullptr);

    ~LocalServer();

private slots:

    void newConnection();

    void readyRead();

    void socketError();

private:
    void passServerRole();

    void sendBodyToNewbie();

    void applyChangesToDocument(const QByteArray& data);

private slots:
    void documentChanged(int position, int charRemoved, int charAdded);

private:
    QLocalServer m_server;
    QLocalSocket m_socket;
    QVector<QLocalSocket*> m_sockets;

    TextEdit& m_textEdit;
    QString m_name;

    bool m_serverMode = false;
};

#endif // LOCALSERVER_H
