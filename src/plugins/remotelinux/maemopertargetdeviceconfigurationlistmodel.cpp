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
#include "maemopertargetdeviceconfigurationlistmodel.h"

#include "linuxdeviceconfigurations.h"
#include "qt4maemotarget.h"

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

MaemoPerTargetDeviceConfigurationListModel::MaemoPerTargetDeviceConfigurationListModel(QObject *parent,
    const Target *target) : QAbstractListModel(parent)
{
    if (qobject_cast<const Qt4Maemo5Target *>(target))
        m_targetOsType = LinuxDeviceConfiguration::Maemo5OsType;
    else if (qobject_cast<const Qt4HarmattanTarget *>(target))
        m_targetOsType = LinuxDeviceConfiguration::HarmattanOsType;
    else if (qobject_cast<const Qt4MeegoTarget *>(target))
        m_targetOsType = LinuxDeviceConfiguration::MeeGoOsType;
    else
        m_targetOsType = LinuxDeviceConfiguration::GenericLinuxOsType;
    const LinuxDeviceConfigurations * const devConfs
        = LinuxDeviceConfigurations::instance();
    connect(devConfs, SIGNAL(modelReset()), this, SIGNAL(modelReset()));
    connect(devConfs, SIGNAL(updated()), this, SIGNAL(updated()));
}

MaemoPerTargetDeviceConfigurationListModel::~MaemoPerTargetDeviceConfigurationListModel()
{
}

int MaemoPerTargetDeviceConfigurationListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    int count = 0;
    const LinuxDeviceConfigurations * const devConfs
        = LinuxDeviceConfigurations::instance();
    const int devConfsCount = devConfs->rowCount();
    if (m_targetOsType == LinuxDeviceConfiguration::GenericLinuxOsType)
        return devConfsCount;
    for (int i = 0; i < devConfsCount; ++i) {
        if (devConfs->deviceAt(i)->osType() == m_targetOsType)
            ++count;
    }
    return count;
}

QVariant MaemoPerTargetDeviceConfigurationListModel::data(const QModelIndex &index,
    int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || role != Qt::DisplayRole)
        return QVariant();
    const LinuxDeviceConfiguration::ConstPtr &devConf = deviceAt(index.row());
    Q_ASSERT(devConf);
    QString displayedName = devConf->name();
    if (devConf->isDefault() && devConf->osType() == m_targetOsType)
        displayedName += QLatin1Char(' ') + tr("(default)");
    return displayedName;
}

LinuxDeviceConfiguration::ConstPtr MaemoPerTargetDeviceConfigurationListModel::deviceAt(int idx) const
{
    int currentRow = -1;
    const LinuxDeviceConfigurations * const devConfs
        = LinuxDeviceConfigurations::instance();
    if (m_targetOsType == LinuxDeviceConfiguration::GenericLinuxOsType)
        return devConfs->deviceAt(idx);
    const int devConfsCount = devConfs->rowCount();
    for (int i = 0; i < devConfsCount; ++i) {
        if (devConfs->deviceAt(i)->osType() == m_targetOsType) {
            if (++currentRow == idx)
                return devConfs->deviceAt(i);
        }
    }
    Q_ASSERT(false);
    return LinuxDeviceConfiguration::ConstPtr();
}

LinuxDeviceConfiguration::ConstPtr MaemoPerTargetDeviceConfigurationListModel::defaultDeviceConfig() const
{
    return LinuxDeviceConfigurations::instance()->defaultDeviceConfig(m_targetOsType);
}

LinuxDeviceConfiguration::ConstPtr MaemoPerTargetDeviceConfigurationListModel::find(LinuxDeviceConfiguration::Id id) const
{
    const LinuxDeviceConfiguration::ConstPtr &devConf
        = LinuxDeviceConfigurations::instance()->find(id);
    return devConf && (devConf->osType() == m_targetOsType
            || m_targetOsType == LinuxDeviceConfiguration::GenericLinuxOsType)
        ? devConf : defaultDeviceConfig();
}

int MaemoPerTargetDeviceConfigurationListModel::indexForInternalId(LinuxDeviceConfiguration::Id id) const
{
    const int count = rowCount();
    for (int i = 0; i < count; ++i) {
        if (deviceAt(i)->internalId() == id)
            return i;
    }
    return -1;
}

} // namespace Internal
} // namespace RemoteLinux
