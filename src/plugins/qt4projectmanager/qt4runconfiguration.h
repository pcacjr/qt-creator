/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QT4RUNCONFIGURATION_H
#define QT4RUNCONFIGURATION_H

#include <projectexplorer/applicationrunconfiguration.h>

#include <utils/environment.h>

#include <QtCore/QStringList>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QRadioButton;
class QComboBox;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
class DebuggerLanguageChooser;
class DetailsWidget;
}

namespace ProjectExplorer {
    class EnvironmentWidget;
}

namespace Qt4ProjectManager {
class Qt4Project;
class Qt4Target;

namespace Internal {
class Qt4PriFileNode;
class Qt4ProFileNode;
class Qt4RunConfigurationFactory;

class Qt4RunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
    // to change the display name and arguments and set the userenvironmentchanges
    friend class Qt4RunConfigurationWidget;
    friend class Qt4RunConfigurationFactory;

public:
    Qt4RunConfiguration(Qt4Target *parent, const QString &proFilePath);
    virtual ~Qt4RunConfiguration();

    Qt4Target *qt4Target() const;

    virtual bool isEnabled(ProjectExplorer::BuildConfiguration *configuration) const;
    using ProjectExplorer::LocalApplicationRunConfiguration::isEnabled;
    virtual QWidget *createConfigurationWidget();

    virtual QString executable() const;
    virtual RunMode runMode() const;
    virtual QString workingDirectory() const;
    virtual QString commandLineArguments() const;
    virtual Utils::Environment environment() const;
    virtual QString dumperLibrary() const;
    virtual QStringList dumperLibraryLocations() const;
    virtual ProjectExplorer::ToolChainType toolChainType() const;

    bool isUsingDyldImageSuffix() const;
    void setUsingDyldImageSuffix(bool state);

    QString proFilePath() const;

    // TODO detectQtShadowBuild() ? how did this work ?
    QVariantMap toMap() const;

    ProjectExplorer::OutputFormatter *createOutputFormatter() const;

signals:
    void commandLineArgumentsChanged(const QString&);
    void baseWorkingDirectoryChanged(const QString&);
    void runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode runMode);
    void usingDyldImageSuffixChanged(bool);
    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);

    // Note: These signals might not get emitted for every change!
    void effectiveTargetInformationChanged();

private slots:
    void proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *pro, bool success);
    void proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *pro);

protected:
    Qt4RunConfiguration(Qt4Target *parent, Qt4RunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);

private:
    void handleParseState(bool success);
    void setRunMode(RunMode runMode);
    void setBaseWorkingDirectory(const QString &workingDirectory);
    QString baseWorkingDirectory() const;
    void setCommandLineArguments(const QString &argumentsString);
    QString rawCommandLineArguments() const;
    enum BaseEnvironmentBase { CleanEnvironmentBase = 0,
                               SystemEnvironmentBase = 1,
                               BuildEnvironmentBase  = 2 };
    QString defaultDisplayName();
    void setBaseEnvironmentBase(BaseEnvironmentBase env);
    BaseEnvironmentBase baseEnvironmentBase() const;

    void ctor();

    Utils::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;

    void updateTarget();
    QString m_commandLineArguments;
    QString m_proFilePath; // Full path to the Application Pro File

    // Cached startup sub project information
    ProjectExplorer::LocalApplicationRunConfiguration::RunMode m_runMode;
    bool m_userSetName;
    bool m_isUsingDyldImageSuffix;
    QString m_userWorkingDirectory;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    BaseEnvironmentBase m_baseEnvironmentBase;
    bool m_parseSuccess;
};

class Qt4RunConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    Qt4RunConfigurationWidget(Qt4RunConfiguration *qt4runconfigration, QWidget *parent);
    ~Qt4RunConfigurationWidget();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private slots:
    void runConfigurationEnabledChange(bool);
    void workDirectoryEdited();
    void workingDirectoryReseted();
    void argumentsEdited(const QString &arguments);
    void userChangesEdited();

    void workingDirectoryChanged(const QString &workingDirectory);
    void commandLineArgumentsChanged(const QString &args);
    void runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode runMode);
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges);
    void baseEnvironmentChanged();

    void effectiveTargetInformationChanged();
    void termToggled(bool);
    void usingDyldImageSuffixToggled(bool);
    void usingDyldImageSuffixChanged(bool);
    void baseEnvironmentSelected(int index);
    void useCppDebuggerToggled(bool toggled);
    void useQmlDebuggerToggled(bool toggled);
    void qmlDebugServerPortChanged(uint port);

private:
    Qt4RunConfiguration *m_qt4RunConfiguration;
    bool m_ignoreChange;
    QLineEdit *m_executableLineEdit;
    Utils::PathChooser *m_workingDirectoryEdit;
    QLineEdit *m_argumentsLineEdit;
    QCheckBox *m_useTerminalCheck;
    QCheckBox *m_usingDyldImageSuffix;
    QLineEdit *m_qmlDebugPort;

    QComboBox *m_baseEnvironmentComboBox;
    Utils::DetailsWidget *m_detailsContainer;
    Utils::DebuggerLanguageChooser *m_debuggerLanguageChooser;
    ProjectExplorer::EnvironmentWidget *m_environmentWidget;
    bool m_isShown;
};

class Qt4RunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit Qt4RunConfigurationFactory(QObject *parent = 0);
    virtual ~Qt4RunConfigurationFactory();

    virtual bool canCreate(ProjectExplorer::Target *parent, const QString &id) const;
    virtual ProjectExplorer::RunConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    virtual bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    virtual ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    virtual bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    virtual ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source);

    QStringList availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(const QString &id) const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4RUNCONFIGURATION_H
