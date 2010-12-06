/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#ifndef MAEMOPUBLISHERFREMANTLEFREE_H
#define MAEMOPUBLISHERFREMANTLEFREE_H

#include <coreplugin/ssh/sshremoteprocessrunner.h>

#include <QtCore/QObject>
#include <QtCore/QProcess>

namespace ProjectExplorer {
class Project;
}

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
namespace Internal {

class MaemoPublisherFremantleFree : public QObject
{
    Q_OBJECT
public:
    enum OutputType {
        StatusOutput, ErrorOutput, ToolStatusOutput, ToolErrorOutput
    };

    explicit MaemoPublisherFremantleFree(const ProjectExplorer::Project *project,
        QObject *parent = 0);
    ~MaemoPublisherFremantleFree();

    void publish();
    void cancel();

    void setBuildConfiguration(const Qt4BuildConfiguration *buildConfig) { m_buildConfig = buildConfig; }
    void setDoUpload(bool doUpload) { m_doUpload = doUpload; }
    void setSshParams(const QString &hostName, const QString &userName,
        const QString &keyFile, const QString &remoteDir);

    QString resultString() const { return m_resultString; }

signals:
    void progressReport(const QString &text,
        MaemoPublisherFremantleFree::OutputType = StatusOutput);
    void finished();

private slots:
    void handleProcessFinished();
    void handleProcessStdOut();
    void handleProcessStdErr();
    void handleProcessError(QProcess::ProcessError error);
    void handleScpStarted();
    void handleConnectionError();
    void handleUploadJobFinished(int exitStatus);
    void handleScpStdOut(const QByteArray &output);

private:
    enum State {
        Inactive, CopyingProjectDir, RunningQmake, RunningMakeDistclean,
        BuildingPackage, StartingScp, PreparingToUploadFile, UploadingFile
    };

    void setState(State newState);
    void createPackage();
    void uploadPackage();
    bool copyRecursively(const QString &srcFilePath,
        const QString &tgtFilePath);
    void handleProcessFinished(bool failedToStart);
    void runDpkgBuildPackage();
    QString tmpDirContainer() const;
    void prepareToSendFile();
    void sendFile();
    void finishWithFailure(const QString &progressMsg, const QString &resultMsg);

    const ProjectExplorer::Project * const m_project;
    bool m_doUpload;
    const Qt4BuildConfiguration *m_buildConfig;
    State m_state;
    QString m_tmpProjectDir;
    QProcess *m_process;
    Core::SshConnectionParameters m_sshParams;
    QString m_remoteDir;
    QSharedPointer<Core::SshRemoteProcessRunner> m_uploader;
    QByteArray m_scpOutput;
    QList<QString> m_filesToUpload;
    QString m_resultString;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOPUBLISHERFREMANTLEFREE_H
