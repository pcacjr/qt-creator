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

#include "remotelinuxrunconfiguration.h"

#include "abstractlinuxdevicedeploystep.h"
#include "maemodeployables.h"
#include "maemoglobal.h"
#include "maemoqemumanager.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfigurationwidget.h"
#include "maemotoolchain.h"
#include "qt4maemodeployconfiguration.h"
#include "qt4maemotarget.h"
#include "maemoqtversion.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <debugger/debuggerconstants.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <qtsupport/qtoutputformatter.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QtCore/QStringBuilder>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxRunConfigurationPrivate {
public:
    RemoteLinuxRunConfigurationPrivate(const QString &proFilePath, const Qt4BaseTarget *target)
        : proFilePath(proFilePath),
          baseEnvironmentType(RemoteLinuxRunConfiguration::SystemBaseEnvironment),
          validParse(target->qt4Project()->validParse(proFilePath)),
          parseInProgress(target->qt4Project()->parseInProgress(proFilePath))
    {
    }

    RemoteLinuxRunConfigurationPrivate(const RemoteLinuxRunConfigurationPrivate *other)
        : proFilePath(other->proFilePath), gdbPath(other->gdbPath), arguments(other->arguments),
          baseEnvironmentType(other->baseEnvironmentType),
          systemEnvironment(other->systemEnvironment),
          userEnvironmentChanges(other->userEnvironmentChanges),
          validParse(other->validParse),
          parseInProgress(other->parseInProgress)
    {
    }

    QString proFilePath;
    QString gdbPath;
    MaemoRemoteMountsModel *remoteMounts;
    QString arguments;
    RemoteLinuxRunConfiguration::BaseEnvironmentType baseEnvironmentType;
    Utils::Environment systemEnvironment;
    QList<Utils::EnvironmentItem> userEnvironmentChanges;
    bool validParse;
    bool parseInProgress;
    QString disabledReason;
};
} // namespace Internal

using namespace Internal;

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Qt4BaseTarget *parent,
        const QString &proFilePath)
    : RunConfiguration(parent, QLatin1String(MAEMO_RC_ID)),
      m_d(new RemoteLinuxRunConfigurationPrivate(proFilePath, parent))
{
    init();
}

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Qt4BaseTarget *parent,
        RemoteLinuxRunConfiguration *source)
    : RunConfiguration(parent, source),
      m_d(new RemoteLinuxRunConfigurationPrivate(source->m_d))
{
    init();
}

void RemoteLinuxRunConfiguration::init()
{
    setDefaultDisplayName(defaultDisplayName());
    setUseCppDebugger(true);
    setUseQmlDebugger(false);
    m_d->remoteMounts = new MaemoRemoteMountsModel(this);

    connect(target(),
        SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
        this, SLOT(handleDeployConfigChanged()));
    handleDeployConfigChanged();

    Qt4Project *pro = qt4Target()->qt4Project();
    connect(pro, SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)),
            this, SLOT(proFileUpdate(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)));
    connect(this, SIGNAL(debuggersChanged()), SLOT(updateEnabledState()));
    connect(m_d->remoteMounts, SIGNAL(rowsInserted(QModelIndex, int, int)), this,
        SLOT(handleRemoteMountsChanged()));
    connect(m_d->remoteMounts, SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
        SLOT(handleRemoteMountsChanged()));
    connect(m_d->remoteMounts, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this,
        SLOT(handleRemoteMountsChanged()));
    connect(m_d->remoteMounts, SIGNAL(modelReset()), SLOT(handleRemoteMountsChanged()));
    connect(target(), SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
        SLOT(updateEnabledState()));
}

RemoteLinuxRunConfiguration::~RemoteLinuxRunConfiguration()
{
}

Qt4BaseTarget *RemoteLinuxRunConfiguration::qt4Target() const
{
    return static_cast<Qt4BaseTarget *>(target());
}

Qt4BuildConfiguration *RemoteLinuxRunConfiguration::activeQt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(activeBuildConfiguration());
}

bool RemoteLinuxRunConfiguration::isEnabled() const
{
    if (m_d->parseInProgress) {
        m_d->disabledReason = tr("The .pro file is being parsed.");
        return false;
    }
    if (!m_d->validParse) {
        m_d->disabledReason = tr("The .pro file could not be parsed.");
        return false;
    }
    if (!deviceConfig()) {
        m_d->disabledReason = tr("No device configuration set.");
        return false;
    }
    if (!activeQt4BuildConfiguration()) {
        m_d->disabledReason = tr("No active build configuration.");
        return false;
    }
    if (remoteExecutableFilePath().isEmpty()) {
        m_d->disabledReason = tr("Don't know what to run.");
        return false;
    }
    if (!hasEnoughFreePorts(ProjectExplorer::Constants::RUNMODE)) {
        m_d->disabledReason = tr("Not enough free ports on the device.");
        return false;
    }
    m_d->disabledReason.clear();
    return true;
}

QString RemoteLinuxRunConfiguration::disabledReason() const
{
    return m_d->disabledReason;
}

QWidget *RemoteLinuxRunConfiguration::createConfigurationWidget()
{
    return new MaemoRunConfigurationWidget(this);
}

Utils::OutputFormatter *RemoteLinuxRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(qt4Target()->qt4Project());
}

void RemoteLinuxRunConfiguration::proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success, bool parseInProgress)
{
    if (m_d->proFilePath == pro->path()) {
        bool enabled = isEnabled();
        m_d->validParse = success;
        m_d->parseInProgress = parseInProgress;
        if (enabled != isEnabled())
            updateEnabledState();
        if (!parseInProgress)
            emit targetInformationChanged();
    }
}

QVariantMap RemoteLinuxRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(ArgumentsKey, m_d->arguments);
    const QDir dir = QDir(target()->project()->projectDirectory());
    map.insert(ProFileKey, dir.relativeFilePath(m_d->proFilePath));
    map.insert(BaseEnvironmentBaseKey, m_d->baseEnvironmentType);
    map.insert(UserEnvironmentChangesKey,
        Utils::EnvironmentItem::toStringList(m_d->userEnvironmentChanges));
    map.unite(m_d->remoteMounts->toMap());
    return map;
}

bool RemoteLinuxRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    m_d->arguments = map.value(ArgumentsKey).toString();
    const QDir dir = QDir(target()->project()->projectDirectory());
    m_d->proFilePath = dir.filePath(map.value(ProFileKey).toString());
    m_d->userEnvironmentChanges =
        Utils::EnvironmentItem::fromStringList(map.value(UserEnvironmentChangesKey)
        .toStringList());
    m_d->baseEnvironmentType = static_cast<BaseEnvironmentType> (map.value(BaseEnvironmentBaseKey,
        SystemBaseEnvironment).toInt());
    m_d->remoteMounts->fromMap(map);

    m_d->validParse = qt4Target()->qt4Project()->validParse(m_d->proFilePath);
    m_d->parseInProgress = qt4Target()->qt4Project()->parseInProgress(m_d->proFilePath);

    setDefaultDisplayName(defaultDisplayName());

    return true;
}

QString RemoteLinuxRunConfiguration::defaultDisplayName()
{
    if (!m_d->proFilePath.isEmpty())
        return (QFileInfo(m_d->proFilePath).completeBaseName()) + QLatin1String(" (remote)");
    //: Remote Linux run configuration default display name
    return tr("Run on remote device");
}

LinuxDeviceConfiguration::ConstPtr RemoteLinuxRunConfiguration::deviceConfig() const
{
    const AbstractLinuxDeviceDeployStep * const step = deployStep();
    return step ? step->helper().deviceConfig() : LinuxDeviceConfiguration::ConstPtr();
}

QString RemoteLinuxRunConfiguration::gdbCmd() const
{
    return QDir::toNativeSeparators(activeBuildConfiguration()->toolChain()->debuggerCommand());
}

Qt4MaemoDeployConfiguration *RemoteLinuxRunConfiguration::deployConfig() const
{
    return qobject_cast<Qt4MaemoDeployConfiguration *>(target()->activeDeployConfiguration());
}

MaemoRemoteMountsModel *RemoteLinuxRunConfiguration::remoteMounts() const
{
    return m_d->remoteMounts;
}

AbstractLinuxDeviceDeployStep *RemoteLinuxRunConfiguration::deployStep() const
{
    return MaemoGlobal::earlierBuildStep<AbstractLinuxDeviceDeployStep>(deployConfig(), 0);
}

QString RemoteLinuxRunConfiguration::arguments() const
{
    return m_d->arguments;
}

QString RemoteLinuxRunConfiguration::commandPrefix() const
{
    if (!deviceConfig())
        return QString();

    return QString::fromLocal8Bit("%1 %2")
        .arg(MaemoGlobal::remoteCommandPrefix(deviceConfig()->osType()))
        .arg(MaemoGlobal::remoteEnvironment(userEnvironmentChanges()));
}

QString RemoteLinuxRunConfiguration::localExecutableFilePath() const
{
    TargetInformation ti = qt4Target()->qt4Project()->rootProjectNode()
        ->targetInformation(m_d->proFilePath);
    if (!ti.valid)
        return QString();

    return QDir::cleanPath(ti.workingDir + QLatin1Char('/') + ti.target);
}

QString RemoteLinuxRunConfiguration::remoteExecutableFilePath() const
{
    return deployConfig()->deployables()->remoteExecutableFilePath(localExecutableFilePath());
}

PortList RemoteLinuxRunConfiguration::freePorts() const
{
    const Qt4BuildConfiguration * const bc = activeQt4BuildConfiguration();
    const AbstractLinuxDeviceDeployStep * const step = deployStep();
    return bc && step
        ? MaemoGlobal::freePorts(deployStep()->helper().deviceConfig(), bc->qtVersion())
        : PortList();
}

void RemoteLinuxRunConfiguration::setArguments(const QString &args)
{
    m_d->arguments = args;
}

RemoteLinuxRunConfiguration::DebuggingType RemoteLinuxRunConfiguration::debuggingType() const
{
    const AbstractQt4MaemoTarget * const maemoTarget
        = qobject_cast<AbstractQt4MaemoTarget *>(target());
    if (!maemoTarget || !maemoTarget->allowsQmlDebugging())
        return DebugCppOnly;
    if (useCppDebugger()) {
        if (useQmlDebugger())
            return DebugCppAndQml;
        return DebugCppOnly;
    }
    return DebugQmlOnly;
}

int RemoteLinuxRunConfiguration::portsUsedByDebuggers() const
{
    switch (debuggingType()) {
    case DebugCppOnly:
    case DebugQmlOnly:
        return 1;
    case DebugCppAndQml:
    default:
        return 2;
    }
}

bool RemoteLinuxRunConfiguration::hasEnoughFreePorts(const QString &mode) const
{
    const int freePortCount = freePorts().count();
    const AbstractQt4MaemoTarget * const maemoTarget
        = qobject_cast<AbstractQt4MaemoTarget *>(target());
    const bool remoteMountsAllowed = maemoTarget && maemoTarget->allowsRemoteMounts();
    const int mountDirCount = remoteMountsAllowed
        ? remoteMounts()->validMountSpecificationCount() : 0;
    if (mode == Debugger::Constants::DEBUGMODE)
        return freePortCount >= mountDirCount + portsUsedByDebuggers();
    if (mode == ProjectExplorer::Constants::RUNMODE)
        return freePortCount >= mountDirCount;
    return false;
}

void RemoteLinuxRunConfiguration::updateDeviceConfigurations()
{
    emit deviceConfigurationChanged(target());
    updateEnabledState();
}

void RemoteLinuxRunConfiguration::handleDeployConfigChanged()
{
    Qt4MaemoDeployConfiguration * const activeDeployConf = deployConfig();
    if (activeDeployConf) {
        connect(activeDeployConf->deployables().data(), SIGNAL(modelReset()),
            SLOT(handleDeployablesUpdated()), Qt::UniqueConnection);
        connect(activeDeployConf->stepList(), SIGNAL(stepInserted(int)),
            SLOT(handleDeployConfigChanged()), Qt::UniqueConnection);
        connect(activeDeployConf->stepList(), SIGNAL(stepInserted(int)),
            SLOT(handleDeployConfigChanged()), Qt::UniqueConnection);
        connect(activeDeployConf->stepList(), SIGNAL(stepMoved(int,int)),
            SLOT(handleDeployConfigChanged()), Qt::UniqueConnection);
        connect(activeDeployConf->stepList(), SIGNAL(stepRemoved(int)),
            SLOT(handleDeployConfigChanged()), Qt::UniqueConnection);
        AbstractLinuxDeviceDeployStep * const step
            = MaemoGlobal::earlierBuildStep<AbstractLinuxDeviceDeployStep>(activeDeployConf, 0);
        if (step) {
            connect(&step->helper(), SIGNAL(deviceConfigChanged()),
                SLOT(updateDeviceConfigurations()), Qt::UniqueConnection);
        }
    }

    updateDeviceConfigurations();
    updateEnabledState();
}

void RemoteLinuxRunConfiguration::handleDeployablesUpdated()
{
    emit deploySpecsChanged();
    updateEnabledState();
}

void RemoteLinuxRunConfiguration::handleRemoteMountsChanged()
{
    emit remoteMountsChanged();
    updateEnabledState();
}

QString RemoteLinuxRunConfiguration::baseEnvironmentText() const
{
    if (m_d->baseEnvironmentType == CleanBaseEnvironment)
        return tr("Clean Environment");
    else  if (m_d->baseEnvironmentType == SystemBaseEnvironment)
        return tr("System Environment");
    return QString();
}

RemoteLinuxRunConfiguration::BaseEnvironmentType RemoteLinuxRunConfiguration::baseEnvironmentType() const
{
    return m_d->baseEnvironmentType;
}

void RemoteLinuxRunConfiguration::setBaseEnvironmentType(BaseEnvironmentType env)
{
    if (m_d->baseEnvironmentType != env) {
        m_d->baseEnvironmentType = env;
        emit baseEnvironmentChanged();
    }
}

Utils::Environment RemoteLinuxRunConfiguration::environment() const
{
    Utils::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

Utils::Environment RemoteLinuxRunConfiguration::baseEnvironment() const
{
    return (m_d->baseEnvironmentType == SystemBaseEnvironment ? systemEnvironment()
        : Utils::Environment());
}

QList<Utils::EnvironmentItem> RemoteLinuxRunConfiguration::userEnvironmentChanges() const
{
    return m_d->userEnvironmentChanges;
}

void RemoteLinuxRunConfiguration::setUserEnvironmentChanges(
    const QList<Utils::EnvironmentItem> &diff)
{
    if (m_d->userEnvironmentChanges != diff) {
        m_d->userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

Utils::Environment RemoteLinuxRunConfiguration::systemEnvironment() const
{
    return m_d->systemEnvironment;
}

void RemoteLinuxRunConfiguration::setSystemEnvironment(const Utils::Environment &environment)
{
    if (m_d->systemEnvironment.size() == 0 || m_d->systemEnvironment != environment) {
        m_d->systemEnvironment = environment;
        emit systemEnvironmentChanged();
    }
}

QString RemoteLinuxRunConfiguration::proFilePath() const
{
    return m_d->proFilePath;
}

} // namespace RemoteLinux
