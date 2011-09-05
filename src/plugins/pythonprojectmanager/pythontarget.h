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

#ifndef GENERICTARGET_H
#define GENERICTARGET_H

#include <projectexplorer/target.h>

#include "pythonbuildconfiguration.h"

#include <QtCore/QStringList>
#include <QtCore/QVariantMap>

namespace ProjectExplorer {
class IBuildConfigurationFactory;
} // namespace ProjectExplorer

namespace PythonProjectManager {

namespace Internal {

const char * const GENERIC_DESKTOP_TARGET_ID("PythonProjectManager.PythonTarget");

class PythonProject;
class PythonRunConfiguration;

class PythonTargetFactory;

class PythonTarget : public ProjectExplorer::Target
{
    Q_OBJECT
    friend class PythonTargetFactory;

public:
    explicit PythonTarget(PythonProject *parent);

    ProjectExplorer::BuildConfigWidget *createConfigWidget();

    PythonProject *pythonProject() const;

    PythonBuildConfigurationFactory *buildConfigurationFactory() const;
    PythonBuildConfiguration *activeBuildConfiguration() const;

protected:
    bool fromMap(const QVariantMap &map);

private:
    PythonBuildConfigurationFactory *m_buildConfigurationFactory;
};

class PythonTargetFactory : public ProjectExplorer::ITargetFactory
{
    Q_OBJECT

public:
    explicit PythonTargetFactory(QObject *parent = 0);

    bool supportsTargetId(const QString &id) const;

    QStringList supportedTargetIds(ProjectExplorer::Project *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    PythonTarget *create(ProjectExplorer::Project *parent, const QString &id);
    bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    PythonTarget *restore(ProjectExplorer::Project *parent, const QVariantMap &map);
};

} // namespace Internal

} // namespace PythonProjectManager

#endif // GENERICTARGET_H
