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

#ifndef QMLTASKMANAGER_H
#define QMLTASKMANAGER_H

#include <projectexplorer/task.h>
#include <qmljs/qmljsdocument.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QFutureWatcher>
#include <QtCore/QTimer>

namespace QmlJSEditor {
class QmlJSTextEditorWidget;
}

namespace ProjectExplorer {
class TaskHub;
} // namespace ProjectExplorer

namespace QmlJSEditor {
namespace Internal {

class QmlTaskManager : public QObject
{
    Q_OBJECT
public:
    QmlTaskManager(QObject *parent = 0);

    void extensionsInitialized();

public slots:
    void updateMessages();
    void documentsRemoved(const QStringList path);

private slots:
    void displayResults(int begin, int end);
    void displayAllResults();
    void updateMessagesNow();

private:
    void insertTask(const ProjectExplorer::Task &task);
    void removeTasksForFile(const QString &fileName);
    void removeAllTasks();

private:
    class FileErrorMessages
    {
    public:
        QString fileName;
        QList<QmlJS::DiagnosticMessage> messages;
    };
    static void collectMessages(QFutureInterface<FileErrorMessages> &future,
                                QmlJS::Snapshot snapshot, QStringList files, QStringList importPaths);

private:
    ProjectExplorer::TaskHub *m_taskHub;
    QMap<QString, QList<ProjectExplorer::Task> > m_docsWithTasks;
    QFutureWatcher<FileErrorMessages> m_messageCollector;
    QTimer m_updateDelay;
};

} // Internal
} // QmlJSEditor

#endif // QMLTASKMANAGER_H
