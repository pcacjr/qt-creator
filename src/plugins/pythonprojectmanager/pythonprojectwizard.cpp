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

#include "pythonprojectwizard.h"
#include "filesselectionwizardpage.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/filewizardpage.h>

#include <QtGui/QIcon>

#include <QtGui/QApplication>
#include <QtGui/QStyle>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QtDebug>
#include <QtCore/QCoreApplication>

using namespace PythonProjectManager::Internal;
using namespace Utils;

//////////////////////////////////////////////////////////////////////////////
// PythonProjectWizardDialog
//////////////////////////////////////////////////////////////////////////////

PythonProjectWizardDialog::PythonProjectWizardDialog(QWidget *parent)
    : ProjectExplorer::BaseProjectWizardDialog(parent)
{
    setWindowTitle(tr("New Python Project"));
    setIntroDescription(tr("This wizard generates a Python project."));
}

PythonProjectWizardDialog::~PythonProjectWizardDialog()
{
}

PythonProjectWizard::PythonProjectWizard()
    : Core::BaseFileWizard(parameters())
{
}

PythonProjectWizard::~PythonProjectWizard()
{
}

Core::BaseFileWizardParameters PythonProjectWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    // TODO do something about the ugliness of standard icons in sizes different than 16, 32, 64, 128
    {
        QPixmap icon(22, 22);
        icon.fill(Qt::transparent);
        QPainter p(&icon);
        p.drawPixmap(3, 3, 16, 16, qApp->style()->standardIcon(QStyle::SP_DirIcon).pixmap(16));
        parameters.setIcon(icon);
    }

    parameters.setDisplayName(tr("Python Application"));
    parameters.setId(QLatin1String("QB.Python Application"));
    parameters.setDescription(tr("Creates a Python application with a "
                "stub implementation."));
    parameters.setCategory(QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QCoreApplication::translate("ProjectExplorer", ProjectExplorer::Constants::PROJECT_WIZARD_TR_CATEGORY));
    return parameters;
}

QWizard *PythonProjectWizard::createWizardDialog(QWidget *parent,
                                                  const QString &defaultPath,
                                                  const WizardPageList &extensionPages) const
{
    PythonProjectWizardDialog *wizard = new PythonProjectWizardDialog(parent);
    setupWizard(wizard);

    wizard->setPath(defaultPath);

    foreach (QWizardPage *p, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(wizard, wizard->addPage(p));

    return wizard;
}

Core::GeneratedFiles PythonProjectWizard::generateFiles(const QWizard *w,
                                                         QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const PythonProjectWizardDialog *wizard =
            qobject_cast<const PythonProjectWizardDialog *>(w);
    const QString projectName = wizard->projectName();
    const QString projectPath = wizard->path() + QLatin1Char('/') + projectName;

    const QString setupFileName =
        Core::BaseFileWizard::buildFileName(projectPath, QLatin1String("setup"),
                                            QLatin1String("py"));
    const QString mainFileName =
        Core::BaseFileWizard::buildFileName(projectPath, projectName,
                                            QLatin1String("py"));

    const QString s = QDir(projectPath).relativeFilePath(mainFileName);

    QString projectContents;
    {
        QTextStream out(&projectContents);

        out << "\'\'\' File generated by Qt Creator, version "
            << Core::Constants::IDE_VERSION_LONG << "\'\'\'" << endl
            << endl
            << "from distutils.core import setup" << endl
            << endl
            << "setup(name=\'" << s << "\'" << endl
            << "      version=\'1.0\'," << endl
            << "      py_modules[\'" << s << "\']" << endl
            << "     )" << endl;
    }

    Core::GeneratedFile generatedSetupFile(setupFileName);
    generatedSetupFile.setContents(projectContents);
    generatedSetupFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    QString contents;
    {
        QTextStream out(&contents);

        out
            << "# -*- coding: utf-8 -*-" << endl
            << endl
            << "def main():" << endl
            << "    pass" << endl
            << endl
            << "if __init__ == \"__main__\":" << endl
            << "    main()" << endl;
    }

    Core::GeneratedFile generatedMainFile(mainFileName);
    generatedMainFile.setContents(contents);
    generatedMainFile.setAttributes(
                        Core::GeneratedFile::OpenEditorAttribute);

    Core::GeneratedFiles files;
    files.append(generatedSetupFile);
    files.append(generatedMainFile);

    return files;

}

bool PythonProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage)
{
    Q_UNUSED(w);
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
}
