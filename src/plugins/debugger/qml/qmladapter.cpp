/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmladapter.h"

#include "debuggerstartparameters.h"
#include "qmldebuggerclient.h"
#include "qmljsprivateapi.h"

#include "debuggerengine.h"

#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

class QmlAdapterPrivate
{
public:
    explicit QmlAdapterPrivate(DebuggerEngine *engine)
        : m_engine(engine)
        , m_qmlClient(0)
        , m_connectionAttempts(0)
        , m_maxConnectionAttempts(50) // overall time: 50 x 200ms
        , m_conn(0)
    {
        m_connectionTimer.setInterval(200);
    }

    QWeakPointer<DebuggerEngine> m_engine;
    Internal::QmlDebuggerClient *m_qmlClient;

    QTimer m_connectionTimer;
    int m_connectionAttempts;
    int m_maxConnectionAttempts;
    QDeclarativeDebugConnection *m_conn;
    QList<QByteArray> sendBuffer;
};

} // namespace Internal

QmlAdapter::QmlAdapter(DebuggerEngine *engine, QObject *parent)
    : QObject(parent), d(new Internal::QmlAdapterPrivate(engine))
{
    connect(&d->m_connectionTimer, SIGNAL(timeout()), SLOT(pollInferior()));
    d->m_conn = new QDeclarativeDebugConnection(this);
    connect(d->m_conn, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            SLOT(connectionStateChanged()));
    connect(d->m_conn, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(connectionErrorOccurred(QAbstractSocket::SocketError)));

}

QmlAdapter::~QmlAdapter()
{
}

void QmlAdapter::beginConnection()
{
    d->m_connectionAttempts = 0;
    d->m_connectionTimer.start();
}

void QmlAdapter::closeConnection()
{
    if (d->m_connectionTimer.isActive()) {
        d->m_connectionTimer.stop();
    } else {
        if (d->m_conn) {
            d->m_conn->close();
        }
    }
}

void QmlAdapter::pollInferior()
{
    ++d->m_connectionAttempts;

    if (isConnected()) {
        d->m_connectionTimer.stop();
        d->m_connectionAttempts = 0;
    } else if (d->m_connectionAttempts == d->m_maxConnectionAttempts) {
        d->m_connectionTimer.stop();
        d->m_connectionAttempts = 0;
        emit connectionStartupFailed();
    } else {
        connectToViewer();
    }
}

void QmlAdapter::connectToViewer()
{
    if (d->m_engine.isNull()
            || (d->m_conn && d->m_conn->state() != QAbstractSocket::UnconnectedState))
        return;

    const DebuggerStartParameters &parameters = d->m_engine.data()->startParameters();
    if (parameters.communicationChannel == DebuggerStartParameters::CommunicationChannelUsb) {
        const QString &port = parameters.remoteChannel;
        showConnectionStatusMessage(tr("Connecting to debug server on %1").arg(port));
        d->m_conn->connectToOst(port);
    } else {
        const QString &address = parameters.qmlServerAddress;
        quint16 port = parameters.qmlServerPort;
        showConnectionStatusMessage(tr("Connecting to debug server %1:%2").arg(address).arg(QString::number(port)));
        d->m_conn->connectToHost(address, port);
    }
}

void QmlAdapter::sendMessage(const QByteArray &msg)
{
    if (d->m_qmlClient->status() == QDeclarativeDebugClient::Enabled) {
        flushSendBuffer();
        d->m_qmlClient->sendMessage(msg);
    } else {
        d->sendBuffer.append(msg);
    }
}

void QmlAdapter::connectionErrorOccurred(QAbstractSocket::SocketError socketError)
{
    showConnectionStatusMessage(tr("Error: (%1) %2", "%1=error code, %2=error message")
                                .arg(socketError).arg(d->m_conn->errorString()));

    // this is only an error if we are already connected and something goes wrong.
    if (isConnected())
        emit connectionError(socketError);
}

void QmlAdapter::clientStatusChanged(QDeclarativeDebugClient::Status status)
{
    QString serviceName;
    if (QDeclarativeDebugClient *client = qobject_cast<QDeclarativeDebugClient*>(sender()))
        serviceName = client->name();

    logServiceStatusChange(serviceName, status);

    if (status == QDeclarativeDebugClient::Enabled)
        flushSendBuffer();
}

void QmlAdapter::connectionStateChanged()
{
    switch (d->m_conn->state()) {
    case QAbstractSocket::UnconnectedState:
    {
        showConnectionStatusMessage(tr("disconnected.\n\n"));
        emit disconnected();

        break;
    }
    case QAbstractSocket::HostLookupState:
        showConnectionStatusMessage(tr("resolving host..."));
        break;
    case QAbstractSocket::ConnectingState:
        showConnectionStatusMessage(tr("connecting to debug server..."));
        break;
    case QAbstractSocket::ConnectedState:
    {
        showConnectionStatusMessage(tr("connected.\n"));

        createDebuggerClient();
        //reloadEngines();
        emit connected();
        break;
    }
    case QAbstractSocket::ClosingState:
        showConnectionStatusMessage(tr("closing..."));
        break;
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
        break;
    }
}

void QmlAdapter::createDebuggerClient()
{
    d->m_qmlClient = new Internal::QmlDebuggerClient(d->m_conn);

    connect(d->m_qmlClient, SIGNAL(newStatus(QDeclarativeDebugClient::Status)),
            this, SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));
    connect(d->m_engine.data(), SIGNAL(sendMessage(QByteArray)),
            this, SLOT(sendMessage(QByteArray)));
    connect(d->m_qmlClient, SIGNAL(messageWasReceived(QByteArray)),
            d->m_engine.data(), SLOT(messageReceived(QByteArray)));

    //engine->startSuccessful();  // FIXME: AAA: port to new debugger states
}

bool QmlAdapter::isConnected() const
{
    return d->m_conn && d->m_qmlClient && d->m_conn->state() == QAbstractSocket::ConnectedState;
}

QDeclarativeDebugConnection *QmlAdapter::connection() const
{
    if (!isConnected())
        return 0;

    return d->m_conn;
}

void QmlAdapter::showConnectionStatusMessage(const QString &message)
{
    if (!d->m_engine.isNull())
        d->m_engine.data()->showMessage(QLatin1String("QmlJSDebugger: ") + message, LogStatus);
}

void QmlAdapter::showConnectionErrorMessage(const QString &message)
{
    if (!d->m_engine.isNull())
        d->m_engine.data()->showMessage(QLatin1String("QmlJSDebugger: ") + message, LogError);
}

bool QmlAdapter::disableJsDebugging(bool block)
{
    if (d->m_engine.isNull())
        return block;

    bool isBlocked = d->m_engine.data()->state() == InferiorRunOk;

    if (isBlocked == block)
        return block;

    if (block) {
        d->m_engine.data()->continueInferior();
    } else {
        d->m_engine.data()->requestInterruptInferior();
    }

    return isBlocked;
}

void QmlAdapter::logServiceStatusChange(const QString &service,
                                        QDeclarativeDebugClient::Status newStatus)
{
    switch (newStatus) {
    case QDeclarativeDebugClient::Unavailable: {
        showConnectionStatusMessage(tr("Debug service '%1' became unavailable.").arg(service));
        emit serviceConnectionError(service);
        break;
    }
    case QDeclarativeDebugClient::Enabled: {
        showConnectionStatusMessage(tr("Connected to debug service '%1'.").arg(service));
        break;
    }

    case QDeclarativeDebugClient::NotConnected: {
        showConnectionStatusMessage(tr("Not connected to debug service '%1'.").arg(service));
        break;
    }
    }
}

void QmlAdapter::logServiceActivity(const QString &service, const QString &logMessage)
{
    if (!d->m_engine.isNull())
        d->m_engine.data()->showMessage(QString("%1 %2").arg(service, logMessage), LogDebug);
}

void QmlAdapter::flushSendBuffer()
{
    QTC_ASSERT(d->m_qmlClient->status() == QDeclarativeDebugClient::Enabled, return);
    foreach (const QByteArray &msg, d->sendBuffer)
        d->m_qmlClient->sendMessage(msg);
    d->sendBuffer.clear();
}

} // namespace Debugger
