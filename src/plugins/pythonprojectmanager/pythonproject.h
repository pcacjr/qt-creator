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

#ifndef GENERICPROJECT_H
#define GENERICPROJECT_H

#include "pythonprojectmanager.h"
#include "pythonprojectnodes.h"
#include "pythontarget.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/ifile.h>

#include <QtCore/QFuture>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
}

namespace ProjectExplorer {
class ToolChain;
}

namespace PythonProjectManager {
namespace Internal {
class PythonBuildConfiguration;
class PythonProject;
class PythonTarget;
class PythonMakeStep;
class PythonProjectFile;

class PythonProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    PythonProject(Manager *manager, const QString &filename);
    virtual ~PythonProject();

    QString filesFileName() const;
    QString includesFileName() const;
    QString configFileName() const;

    QString displayName() const;
    QString id() const;
    Core::IFile *file() const;
    ProjectExplorer::IProjectManager *projectManager() const;
    PythonTarget *activeTarget() const;

    QList<ProjectExplorer::Project *> dependsOn();

    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    PythonProjectNode *rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;

    QStringList buildTargets() const;

    bool addFiles(const QStringList &filePaths);
    bool removeFiles(const QStringList &filePaths);
    bool setFiles(const QStringList &filePaths);

    enum RefreshOptions {
        Files         = 0x01,
        Configuration = 0x02,
        Everything    = Files | Configuration
    };

    void refresh(RefreshOptions options);

    QStringList includePaths() const;
    void setIncludePaths(const QStringList &includePaths);

    QByteArray defines() const;
    QStringList allIncludePaths() const;
    QStringList projectIncludePaths() const;
    QStringList files() const;
    QStringList generated() const;
    ProjectExplorer::ToolChain *toolChain() const;
    void setToolChain(ProjectExplorer::ToolChain *tc);

    QVariantMap toMap() const;

signals:
    void toolChainChanged(ProjectExplorer::ToolChain *);

protected:
    virtual bool fromMap(const QVariantMap &map);

private:
    bool saveRawFileList(const QStringList &rawFileList);
    void parseProject(RefreshOptions options);
    QStringList processEntries(const QStringList &paths,
                               QHash<QString, QString> *map = 0) const;

    Manager *m_manager;
    QString m_fileName;
    QString m_filesFileName;
    QString m_includesFileName;
    QString m_configFileName;
    PythonProjectFile *m_file;
    QString m_projectName;

    QStringList m_rawFileList;
    QStringList m_files;
    QHash<QString, QString> m_rawListEntries;
    QStringList m_generated;
    QStringList m_includePaths;
    QStringList m_projectIncludePaths;
    QByteArray m_defines;

    PythonProjectNode *m_rootNode;
    ProjectExplorer::ToolChain *m_toolChain;
    QFuture<void> m_codeModelFuture;
};

class PythonProjectFile : public Core::IFile
{
    Q_OBJECT

public:
    PythonProjectFile(PythonProject *parent, QString fileName);
    virtual ~PythonProjectFile();

    virtual bool save(QString *errorString, const QString &fileName, bool autoSave);
    virtual QString fileName() const;

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;

    virtual bool isModified() const;
    virtual bool isReadOnly() const;
    virtual bool isSaveAsAllowed() const;
    virtual void rename(const QString &newName);

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

private:
    PythonProject *m_project;
    QString m_fileName;
};

class PythonBuildSettingsWidget : public ProjectExplorer::BuildConfigWidget
{
    Q_OBJECT

public:
    PythonBuildSettingsWidget(PythonTarget *target);
    virtual ~PythonBuildSettingsWidget();

    virtual QString displayName() const;

    virtual void init(ProjectExplorer::BuildConfiguration *bc);

private Q_SLOTS:
    void buildDirectoryChanged();
    void toolChainSelected(int index);
    void toolChainChanged(ProjectExplorer::ToolChain *);
    void updateToolChainList();

private:
    PythonTarget *m_target;
    Utils::PathChooser *m_pathChooser;
    QComboBox *m_toolChainChooser;
    PythonBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace PythonProjectManager

#endif // GENERICPROJECT_H
