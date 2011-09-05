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

#include "pythonprojectplugin.h"
#include "pythonprojectmanager.h"
#include "pythonprojectwizard.h"
#include "pythonprojectconstants.h"
#include "pythonprojectfileseditor.h"
#include "pythonmakestep.h"
#include "pythontarget.h"
#include "pythonproject.h"
#include "selectablefilesmodel.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>


#include <texteditor/texteditoractionhandler.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>

#include <QtGui/QTreeView>
#include <QtGui/QMainWindow>

using namespace PythonProjectManager;
using namespace PythonProjectManager::Internal;

PythonProjectPlugin::PythonProjectPlugin()
    : m_projectFilesEditorFactory(0)
{ }

PythonProjectPlugin::~PythonProjectPlugin()
{
    removeObject(m_projectFilesEditorFactory);
    delete m_projectFilesEditorFactory;
}

bool PythonProjectPlugin::initialize(const QStringList &, QString *errorMessage)
{
    using namespace Core;

    ICore *core = ICore::instance();
    Core::MimeDatabase *mimeDB = core->mimeDatabase();

    const QLatin1String mimetypesXml(":pythonproject/PythonProject.mimetypes.xml");

    if (! mimeDB->addMimeTypes(mimetypesXml, errorMessage))
        return false;

    Manager *manager = new Manager;

    TextEditor::TextEditorActionHandler *actionHandler =
            new TextEditor::TextEditorActionHandler(Constants::C_FILESEDITOR);

    m_projectFilesEditorFactory = new ProjectFilesFactory(manager, actionHandler);
    addObject(m_projectFilesEditorFactory);

    addAutoReleasedObject(manager);
    addAutoReleasedObject(new PythonMakeStepFactory);
    addAutoReleasedObject(new PythonProjectWizard);
    addAutoReleasedObject(new PythonTargetFactory);

    const Core::Context projectContext(Constants::PROJECTCONTEXT);
    Core::ActionManager *am = core->actionManager();
    Core::ActionContainer *mproject =
            am->actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    m_editFilesAction = new QAction(tr("Edit Files..."), this);
    Core::Command *command = am->registerAction(m_editFilesAction, Constants::EDITFILESACTION, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);
    connect(m_editFilesAction, SIGNAL(triggered()), this, SLOT(editFiles()));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),
            SIGNAL(aboutToShowContextMenu(ProjectExplorer::Project*, ProjectExplorer::Node*)),
            this, SLOT(updateContextMenu(ProjectExplorer::Project*, ProjectExplorer::Node*)));

    return true;
}

void PythonProjectPlugin::extensionsInitialized()
{ }

void PythonProjectPlugin::updateContextMenu(ProjectExplorer::Project *project, ProjectExplorer::Node*)
{
    m_contextMenuProject = project;
}

void PythonProjectPlugin::editFiles()
{
    PythonProject *pythonProject = static_cast<PythonProject *>(m_contextMenuProject);

    Core::MimeDatabase *mimeDatabase = Core::ICore::instance()->mimeDatabase();
    SelectableFilesDialog sfd(QFileInfo(pythonProject->file()->fileName()).path(), pythonProject->files(),
                              mimeDatabase->suffixes().toSet(), Core::ICore::instance()->mainWindow());
    if (sfd.exec() == QDialog::Accepted) {
        pythonProject->setFiles(sfd.selectedFiles());
    }
}

Q_EXPORT_PLUGIN(PythonProjectPlugin)
