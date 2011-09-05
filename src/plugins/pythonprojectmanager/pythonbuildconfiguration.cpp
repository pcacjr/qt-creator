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

#include "pythonbuildconfiguration.h"

#include "pythonmakestep.h"
#include "pythonproject.h"
#include "pythontarget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QtGui/QInputDialog>

using namespace PythonProjectManager;
using namespace PythonProjectManager::Internal;
using ProjectExplorer::BuildConfiguration;

namespace {
const char * const GENERIC_BC_ID("PythonProjectManager.PythonBuildConfiguration");

const char * const BUILD_DIRECTORY_KEY("PythonProjectManager.PythonBuildConfiguration.BuildDirectory");
}

PythonBuildConfiguration::PythonBuildConfiguration(PythonTarget *parent)
    : BuildConfiguration(parent, QLatin1String(GENERIC_BC_ID))
{
}

PythonBuildConfiguration::PythonBuildConfiguration(PythonTarget *parent, const QString &id)
    : BuildConfiguration(parent, id)
{
}

PythonBuildConfiguration::PythonBuildConfiguration(PythonTarget *parent, PythonBuildConfiguration *source) :
    BuildConfiguration(parent, source),
    m_buildDirectory(source->m_buildDirectory)
{
    cloneSteps(source);
}

PythonBuildConfiguration::~PythonBuildConfiguration()
{
}

QVariantMap PythonBuildConfiguration::toMap() const
{
    QVariantMap map(BuildConfiguration::toMap());
    map.insert(QLatin1String(BUILD_DIRECTORY_KEY), m_buildDirectory);
    return map;
}

bool PythonBuildConfiguration::fromMap(const QVariantMap &map)
{
    m_buildDirectory = map.value(QLatin1String(BUILD_DIRECTORY_KEY), target()->project()->projectDirectory()).toString();

    return BuildConfiguration::fromMap(map);
}

QString PythonBuildConfiguration::buildDirectory() const
{
    // Convert to absolute path when necessary
    const QDir projectDir(target()->project()->projectDirectory());
    return projectDir.absoluteFilePath(m_buildDirectory);
}

/**
 * Returns the build directory unmodified, instead of making it absolute like
 * buildDirectory() does.
 */
QString PythonBuildConfiguration::rawBuildDirectory() const
{
    return m_buildDirectory;
}

void PythonBuildConfiguration::setBuildDirectory(const QString &buildDirectory)
{
    if (m_buildDirectory == buildDirectory)
        return;
    m_buildDirectory = buildDirectory;
    emit buildDirectoryChanged();
}

PythonTarget *PythonBuildConfiguration::pythonTarget() const
{
    return static_cast<PythonTarget *>(target());
}

ProjectExplorer::IOutputParser *PythonBuildConfiguration::createOutputParser() const
{
    ProjectExplorer::ToolChain *tc = pythonTarget()->pythonProject()->toolChain();
    if (tc)
        return tc->outputParser();
    return 0;
}


/*!
  \class PythonBuildConfigurationFactory
*/

PythonBuildConfigurationFactory::PythonBuildConfigurationFactory(QObject *parent) :
    ProjectExplorer::IBuildConfigurationFactory(parent)
{
}

PythonBuildConfigurationFactory::~PythonBuildConfigurationFactory()
{
}

QStringList PythonBuildConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!qobject_cast<PythonTarget *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(GENERIC_BC_ID);
}

QString PythonBuildConfigurationFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(GENERIC_BC_ID))
        return tr("Build");
    return QString();
}

bool PythonBuildConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const QString &id) const
{
    if (!qobject_cast<PythonTarget *>(parent))
        return false;
    if (id == QLatin1String(GENERIC_BC_ID))
        return true;
    return false;
}

BuildConfiguration *PythonBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    PythonTarget *target(static_cast<PythonTarget *>(parent));

    //TODO asking for name is duplicated everywhere, but maybe more
    // wizards will show up, that incorporate choosing the name
    bool ok;
    QString buildConfigurationName = QInputDialog::getText(0,
                          tr("New Configuration"),
                          tr("New configuration name:"),
                          QLineEdit::Normal,
                          QString(),
                          &ok);
    if (!ok || buildConfigurationName.isEmpty())
        return 0;
    PythonBuildConfiguration *bc = new PythonBuildConfiguration(target);
    bc->setDisplayName(buildConfigurationName);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    Q_ASSERT(buildSteps);
    PythonMakeStep *makeStep = new PythonMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);
    makeStep->setBuildTarget("all", /* on = */ true);

    target->addBuildConfiguration(bc); // also makes the name unique...
    return bc;
}

bool PythonBuildConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const
{
    return canCreate(parent, source->id());
}

BuildConfiguration *PythonBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    PythonTarget *target(static_cast<PythonTarget *>(parent));
    return new PythonBuildConfiguration(target, qobject_cast<PythonBuildConfiguration *>(source));
}

bool PythonBuildConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

BuildConfiguration *PythonBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    PythonTarget *target(static_cast<PythonTarget *>(parent));
    PythonBuildConfiguration *bc(new PythonBuildConfiguration(target));
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

BuildConfiguration::BuildType PythonBuildConfiguration::buildType() const
{
    return Unknown;
}

