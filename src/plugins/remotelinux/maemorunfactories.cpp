/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "maemorunfactories.h"

#include "maemoconstants.h"
#include "maemodebugsupport.h"
#include "maemoglobal.h"
#include "maemoremotemountsmodel.h"
#include "remotelinuxrunconfiguration.h"
#include "maemoruncontrol.h"
#include "maemotoolchain.h"
#include "qt4maemotarget.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <debugger/debuggerconstants.h>
#include <qt4projectmanager/qt4project.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {

namespace {

QString pathFromId(const QString &id)
{
    if (!id.startsWith(MAEMO_RC_ID_PREFIX))
        return QString();
    return id.mid(QString(MAEMO_RC_ID_PREFIX).size());
}

} // namespace

MaemoRunConfigurationFactory::MaemoRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
}

MaemoRunConfigurationFactory::~MaemoRunConfigurationFactory()
{
}

bool MaemoRunConfigurationFactory::canCreate(Target *parent,
    const QString &id) const
{
    if (!MaemoGlobal::hasLinuxQt(parent))
        return false;
    return qobject_cast<Qt4BaseTarget *>(parent)->qt4Project()
        ->hasApplicationProFile(pathFromId(id));
}

bool MaemoRunConfigurationFactory::canRestore(Target *parent,
    const QVariantMap &map) const
{
    if (!MaemoGlobal::hasLinuxQt(parent))
        return false;
    return ProjectExplorer::idFromMap(map)
        .startsWith(QLatin1String(MAEMO_RC_ID));
}

bool MaemoRunConfigurationFactory::canClone(Target *parent,
    RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

QStringList MaemoRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    if (Qt4BaseTarget *t = qobject_cast<Qt4BaseTarget *>(parent)) {
        if (t && MaemoGlobal::hasLinuxQt(t)) {
            return t->qt4Project()->
                applicationProFilePathes(QLatin1String(MAEMO_RC_ID_PREFIX));
        }
    }
    return QStringList();
}

QString MaemoRunConfigurationFactory::displayNameForId(const QString &id) const
{
    return QFileInfo(pathFromId(id)).completeBaseName()
        + QLatin1String(" (remote)");
}

RunConfiguration *MaemoRunConfigurationFactory::create(Target *parent,
    const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    return new RemoteLinuxRunConfiguration(qobject_cast<Qt4BaseTarget *>(parent),
        pathFromId(id));
}

RunConfiguration *MaemoRunConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    RemoteLinuxRunConfiguration *rc
        = new RemoteLinuxRunConfiguration(qobject_cast<Qt4BaseTarget *>(parent), QString());
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

RunConfiguration *MaemoRunConfigurationFactory::clone(Target *parent,
    RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    RemoteLinuxRunConfiguration *old = static_cast<RemoteLinuxRunConfiguration *>(source);
    return new RemoteLinuxRunConfiguration(static_cast<Qt4BaseTarget *>(parent), old);
}

// #pragma mark -- MaemoRunControlFactory

MaemoRunControlFactory::MaemoRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

MaemoRunControlFactory::~MaemoRunControlFactory()
{
}

bool MaemoRunControlFactory::canRun(RunConfiguration *runConfiguration,
    const QString &mode) const
{
    const RemoteLinuxRunConfiguration * const maemoRunConfig
        = qobject_cast<RemoteLinuxRunConfiguration *>(runConfiguration);
    if (!maemoRunConfig || !maemoRunConfig->isEnabled())
        return false;
    return maemoRunConfig->hasEnoughFreePorts(mode);
}

RunControl* MaemoRunControlFactory::create(RunConfiguration *runConfig,
    const QString &mode)
{
    Q_ASSERT(mode == ProjectExplorer::Constants::RUNMODE
        || mode == Debugger::Constants::DEBUGMODE);
    Q_ASSERT(canRun(runConfig, mode));
    RemoteLinuxRunConfiguration *rc = qobject_cast<RemoteLinuxRunConfiguration *>(runConfig);
    Q_ASSERT(rc);
    if (mode == ProjectExplorer::Constants::RUNMODE)
        return new MaemoRunControl(rc);
    return MaemoDebugSupport::createDebugRunControl(rc);
}

QString MaemoRunControlFactory::displayName() const
{
    return tr("Run on device");
}

RunConfigWidget *MaemoRunControlFactory::createConfigurationWidget(RunConfiguration *config)
{
    Q_UNUSED(config)
    return 0;
}

    } // namespace Internal
} // namespace RemoteLinux
