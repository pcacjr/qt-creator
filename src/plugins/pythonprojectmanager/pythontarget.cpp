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

#include "pythontarget.h"

#include "pythonbuildconfiguration.h"
#include "pythonproject.h"
#include "pythonmakestep.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtGui/QApplication>
#include <QtGui/QStyle>

namespace {
const char * const GENERIC_DESKTOP_TARGET_DISPLAY_NAME("Desktop");
}

using namespace PythonProjectManager;
using namespace PythonProjectManager::Internal;

////////////////////////////////////////////////////////////////////////////////////
// PythonTarget
////////////////////////////////////////////////////////////////////////////////////

PythonTarget::PythonTarget(PythonProject *parent) :
    ProjectExplorer::Target(parent, QLatin1String(GENERIC_DESKTOP_TARGET_ID)),
    m_buildConfigurationFactory(new PythonBuildConfigurationFactory(this))
{
    setDefaultDisplayName(QApplication::translate("PythonProjectManager::PythonTarget",
                                                  GENERIC_DESKTOP_TARGET_DISPLAY_NAME));
    setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
}

ProjectExplorer::BuildConfigWidget *PythonTarget::createConfigWidget()
{
    return new PythonBuildSettingsWidget(this);
}

PythonProject *PythonTarget::pythonProject() const
{
    return static_cast<PythonProject *>(project());
}

PythonBuildConfigurationFactory *PythonTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

PythonBuildConfiguration *PythonTarget::activeBuildConfiguration() const
{
    return static_cast<PythonBuildConfiguration *>(Target::activeBuildConfiguration());
}

bool PythonTarget::fromMap(const QVariantMap &map)
{
    if (!Target::fromMap(map))
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////
// PythonTargetFactory
////////////////////////////////////////////////////////////////////////////////////

PythonTargetFactory::PythonTargetFactory(QObject *parent) :
    ITargetFactory(parent)
{
}

bool PythonTargetFactory::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(GENERIC_DESKTOP_TARGET_ID);
}

QStringList PythonTargetFactory::supportedTargetIds(ProjectExplorer::Project *parent) const
{
    if (!qobject_cast<PythonProject *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(GENERIC_DESKTOP_TARGET_ID);
}

QString PythonTargetFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(GENERIC_DESKTOP_TARGET_ID))
        return QCoreApplication::translate("PythonProjectManager::PythonTarget",
                                           GENERIC_DESKTOP_TARGET_DISPLAY_NAME,
                                           "Python desktop target display name");
    return QString();
}

bool PythonTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<PythonProject *>(parent)) {
        qDebug() << "Cannot create target factory.";
        return false;
    }

    return id == QLatin1String(GENERIC_DESKTOP_TARGET_ID);
}

PythonTarget *PythonTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    PythonProject *pythonproject = static_cast<PythonProject *>(parent);
    PythonTarget *t = new PythonTarget(pythonproject);

    // Set up BuildConfiguration:
    PythonBuildConfiguration *bc = new PythonBuildConfiguration(t);
    bc->setDisplayName("all");

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    PythonMakeStep *makeStep = new PythonMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    makeStep->setBuildTarget("all", /* on = */ true);

    bc->setBuildDirectory(pythonproject->projectDirectory());

    t->addBuildConfiguration(bc);

    t->addDeployConfiguration(t->createDeployConfiguration(ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));

    // Add a runconfiguration. The CustomExecutableRC one will query the user
    // for its settings, so it is a good choice here.
    qDebug() << "Adding runconfiguration...";
    t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));

    return t;
}

bool PythonTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

PythonTarget *PythonTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    PythonProject *pythonproject = static_cast<PythonProject *>(parent);
    PythonTarget *target = new PythonTarget(pythonproject);
    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}
