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

#include "pythonprojectnodes.h"
#include "pythonproject.h"

#include <coreplugin/ifile.h>
#include <projectexplorer/projectexplorer.h>

#include <QtCore/QFileInfo>

using namespace PythonProjectManager;
using namespace PythonProjectManager::Internal;

PythonProjectNode::PythonProjectNode(PythonProject *project, Core::IFile *projectFile)
    : ProjectExplorer::ProjectNode(projectFile->fileName()),
      m_project(project),
      m_projectFile(projectFile)
{
    setDisplayName(QFileInfo(projectFile->fileName()).completeBaseName());
}

Core::IFile *PythonProjectNode::projectFile() const
{
    return m_projectFile;
}

QString PythonProjectNode::projectFilePath() const
{
    return m_projectFile->fileName();
}

void PythonProjectNode::refresh()
{
    using namespace ProjectExplorer;

    // remove the existing nodes.
    removeFileNodes(fileNodes(), this);
    removeFolderNodes(subFolderNodes(), this);

    //ProjectExplorerPlugin::instance()->setCurrentNode(0); // ### remove me

    FileNode *projectFilesNode = new FileNode(m_project->filesFileName(),
                                              ProjectFileType,
                                              /* generated = */ false);

    FileNode *projectIncludesNode = new FileNode(m_project->includesFileName(),
                                                 ProjectFileType,
                                                 /* generated = */ false);

    FileNode *projectConfigNode = new FileNode(m_project->configFileName(),
                                               ProjectFileType,
                                               /* generated = */ false);

    QStringList files = m_project->files();
    files.removeAll(m_project->filesFileName());
    files.removeAll(m_project->includesFileName());
    files.removeAll(m_project->configFileName());

    addFileNodes(QList<FileNode *>()
                 << projectFilesNode
                 << projectIncludesNode
                 << projectConfigNode,
                 this);

    QStringList filePaths;
    QHash<QString, QStringList> filesInPath;
    const QString base = QFileInfo(path()).absolutePath();
    const QDir baseDir(base);

    foreach (const QString &absoluteFileName, files) {
        QFileInfo fileInfo(absoluteFileName);
        const QString absoluteFilePath = fileInfo.path();
        QString relativeFilePath;

        if (absoluteFilePath.startsWith(base)) {
            relativeFilePath = absoluteFilePath.mid(base.length() + 1);
        } else {
            // `file' is not part of the project.
            relativeFilePath = baseDir.relativeFilePath(absoluteFilePath);
        }

        if (! filePaths.contains(relativeFilePath))
            filePaths.append(relativeFilePath);

        filesInPath[relativeFilePath].append(absoluteFileName);
    }

    FolderByName folderByName;
    foreach (const QString &filePath, filePaths) {
        QStringList components = filePath.split(QLatin1Char('/'));
        FolderNode *folder = findOrCreateFolderByName(&folderByName, components, components.size());

        QList<FileNode *> fileNodes;
        foreach (const QString &file, filesInPath.value(filePath)) {
            FileType fileType = SourceType; // ### FIXME
            FileNode *fileNode = new FileNode(file, fileType, /*generated = */ false);
            fileNodes.append(fileNode);
        }

        addFileNodes(fileNodes, folder);
    }
}

ProjectExplorer::FolderNode *PythonProjectNode::findOrCreateFolderByName
    (FolderByName *folderByName, const QStringList &components, int end)
{
    if (!end)
        return 0;

    QString folderName;
    for (int i = 0; i < end; ++i) {
        folderName.append(components.at(i));
        folderName += QLatin1Char('/'); // ### FIXME
    }

    const QString component = components.at(end - 1);

    if (component.isEmpty())
        return this;

    else if (FolderNode *folder = folderByName->value(folderName))
        return folder;

    const QString baseDir = QFileInfo(path()).path();
    FolderNode *folder = new FolderNode(baseDir + QLatin1Char('/') + folderName);
    folder->setDisplayName(component);
    folderByName->insert(folderName, folder);

    FolderNode *parent = findOrCreateFolderByName(folderByName, components, end - 1);
    if (!parent)
        parent = this;
    addFolderNodes(QList<FolderNode*>() << folder, parent);

    return folder;
}

bool PythonProjectNode::hasBuildTargets() const
{
    return true;
}

QList<ProjectExplorer::ProjectNode::ProjectAction> PythonProjectNode::supportedActions(Node *node) const
{
    Q_UNUSED(node);
    return QList<ProjectAction>()
        << AddNewFile
        << AddExistingFile
        << RemoveFile;
}

bool PythonProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool PythonProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool PythonProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool PythonProjectNode::addFiles(const ProjectExplorer::FileType fileType,
                                  const QStringList &filePaths, QStringList *notAdded)
{
    Q_UNUSED(fileType)
    Q_UNUSED(notAdded)

    return m_project->addFiles(filePaths);
}

bool PythonProjectNode::removeFiles(const ProjectExplorer::FileType fileType,
                                     const QStringList &filePaths, QStringList *notRemoved)
{
    Q_UNUSED(fileType)
    Q_UNUSED(notRemoved)

    return m_project->removeFiles(filePaths);
}

bool PythonProjectNode::deleteFiles(const ProjectExplorer::FileType fileType,
                                     const QStringList &filePaths)
{
    Q_UNUSED(fileType)
    Q_UNUSED(filePaths)
    return false;
}

bool PythonProjectNode::renameFile(const ProjectExplorer::FileType fileType,
                                    const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(fileType)
    Q_UNUSED(filePath)
    Q_UNUSED(newFilePath)
    return false;
}

QList<ProjectExplorer::RunConfiguration *> PythonProjectNode::runConfigurationsFor(Node *node)
{
    Q_UNUSED(node)
    return QList<ProjectExplorer::RunConfiguration *>();
}
