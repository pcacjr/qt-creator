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

#include "pythonmakestep.h"
#include "pythonprojectconstants.h"
#include "pythonproject.h"
#include "pythontarget.h"
#include "ui_pythonmakestep.h"
#include "pythonbuildconfiguration.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/gnumakeparser.h>
#include <coreplugin/variablemanager.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>

using namespace PythonProjectManager;
using namespace PythonProjectManager::Internal;

namespace {
const char * const GENERIC_MS_ID("PythonProjectManager.PythonMakeStep");
const char * const GENERIC_MS_DISPLAY_NAME(QT_TRANSLATE_NOOP("PythonProjectManager::Internal::PythonMakeStep",
                                                             "Make"));

const char * const BUILD_TARGETS_KEY("PythonProjectManager.PythonMakeStep.BuildTargets");
const char * const MAKE_ARGUMENTS_KEY("PythonProjectManager.PythonMakeStep.MakeArguments");
const char * const MAKE_COMMAND_KEY("PythonProjectManager.PythonMakeStep.MakeCommand");
}

PythonMakeStep::PythonMakeStep(ProjectExplorer::BuildStepList *parent) :
    AbstractProcessStep(parent, QLatin1String(GENERIC_MS_ID))
{
    ctor();
}

PythonMakeStep::PythonMakeStep(ProjectExplorer::BuildStepList *parent, const QString &id) :
    AbstractProcessStep(parent, id)
{
    ctor();
}

PythonMakeStep::PythonMakeStep(ProjectExplorer::BuildStepList *parent, PythonMakeStep *bs) :
    AbstractProcessStep(parent, bs),
    m_buildTargets(bs->m_buildTargets),
    m_makeArguments(bs->m_makeArguments),
    m_makeCommand(bs->m_makeCommand)
{
    ctor();
}

void PythonMakeStep::ctor()
{
    setDefaultDisplayName(QCoreApplication::translate("PythonProjectManager::Internal::PythonMakeStep",
                                                      GENERIC_MS_DISPLAY_NAME));
}

PythonMakeStep::~PythonMakeStep()
{
}

PythonBuildConfiguration *PythonMakeStep::pythonBuildConfiguration() const
{
    return static_cast<PythonBuildConfiguration *>(buildConfiguration());
}

bool PythonMakeStep::init()
{
    PythonBuildConfiguration *bc = pythonBuildConfiguration();

    setEnabled(true);
    ProjectExplorer::ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory());
    pp->setEnvironment(bc->environment());
    pp->setCommand(makeCommand());
    pp->setArguments(allArguments());

    setOutputParser(new ProjectExplorer::GnuMakeParser());
    if (bc->pythonTarget()->pythonProject()->toolChain())
        appendOutputParser(bc->pythonTarget()->pythonProject()->toolChain()->outputParser());
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init();
}

QVariantMap PythonMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(BUILD_TARGETS_KEY), m_buildTargets);
    map.insert(QLatin1String(MAKE_ARGUMENTS_KEY), m_makeArguments);
    map.insert(QLatin1String(MAKE_COMMAND_KEY), m_makeCommand);
    return map;
}

bool PythonMakeStep::fromMap(const QVariantMap &map)
{
    m_buildTargets = map.value(QLatin1String(BUILD_TARGETS_KEY)).toStringList();
    m_makeArguments = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toString();
    m_makeCommand = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();

    return BuildStep::fromMap(map);
}

QString PythonMakeStep::allArguments() const
{
    QString args = m_makeArguments;
    Utils::QtcProcess::addArgs(&args, m_buildTargets);
    return args;
}

QString PythonMakeStep::makeCommand() const
{
    QString command = m_makeCommand;
    if (command.isEmpty()) {
        PythonProject *pro = pythonBuildConfiguration()->pythonTarget()->pythonProject();
        if (ProjectExplorer::ToolChain *toolChain = pro->toolChain())
            command = toolChain->makeCommand();
        else
            command = QLatin1String("make");
    }
    return command;
}

void PythonMakeStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

ProjectExplorer::BuildStepConfigWidget *PythonMakeStep::createConfigWidget()
{
    return new PythonMakeStepConfigWidget(this);
}

bool PythonMakeStep::immutable() const
{
    return false;
}

bool PythonMakeStep::buildsTarget(const QString &target) const
{
    return m_buildTargets.contains(target);
}

void PythonMakeStep::setBuildTarget(const QString &target, bool on)
{
    QStringList old = m_buildTargets;
    if (on && !old.contains(target))
         old << target;
    else if(!on && old.contains(target))
        old.removeOne(target);

    m_buildTargets = old;
}

//
// PythonMakeStepConfigWidget
//

PythonMakeStepConfigWidget::PythonMakeStepConfigWidget(PythonMakeStep *makeStep)
    : m_makeStep(makeStep)
{
    m_ui = new Ui::PythonMakeStep;
    m_ui->setupUi(this);

    // TODO update this list also on rescans of the PythonLists.txt
    PythonProject *pro = m_makeStep->pythonBuildConfiguration()->pythonTarget()->pythonProject();
    foreach (const QString &target, pro->buildTargets()) {
        QListWidgetItem *item = new QListWidgetItem(target, m_ui->targetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_makeStep->buildsTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }

    m_ui->makeLineEdit->setText(m_makeStep->m_makeCommand);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->m_makeArguments);
    updateMakeOverrrideLabel();
    updateDetails();

    connect(m_ui->targetsList, SIGNAL(itemChanged(QListWidgetItem*)),
            this, SLOT(itemChanged(QListWidgetItem*)));
    connect(m_ui->makeLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeLineEditTextEdited()));
    connect(m_ui->makeArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeArgumentsLineEditTextEdited()));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateMakeOverrrideLabel()));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
}

QString PythonMakeStepConfigWidget::displayName() const
{
    return tr("Make", "PythonMakestep display name.");
}

// TODO: Label should update when tool chain is changed
void PythonMakeStepConfigWidget::updateMakeOverrrideLabel()
{
    m_ui->makeLabel->setText(tr("Override %1:").arg(m_makeStep->makeCommand()));
}

void PythonMakeStepConfigWidget::updateDetails()
{
    PythonBuildConfiguration *bc = m_makeStep->pythonBuildConfiguration();

    ProjectExplorer::ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory());
    param.setEnvironment(bc->environment());
    param.setCommand(m_makeStep->makeCommand());
    param.setArguments(m_makeStep->allArguments());
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}

QString PythonMakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void PythonMakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

void PythonMakeStepConfigWidget::makeLineEditTextEdited()
{
    m_makeStep->m_makeCommand = m_ui->makeLineEdit->text();
    updateDetails();
}

void PythonMakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    m_makeStep->m_makeArguments = m_ui->makeArgumentsLineEdit->text();
    updateDetails();
}

//
// PythonMakeStepFactory
//

PythonMakeStepFactory::PythonMakeStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

PythonMakeStepFactory::~PythonMakeStepFactory()
{
}

bool PythonMakeStepFactory::canCreate(ProjectExplorer::BuildStepList *parent,
                                       const QString &id) const
{
    if (parent->target()->project()->id() != QLatin1String(Constants::GENERICPROJECT_ID))
        return false;
    return id == QLatin1String(GENERIC_MS_ID);
}

ProjectExplorer::BuildStep *PythonMakeStepFactory::create(ProjectExplorer::BuildStepList *parent,
                                                           const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    return new PythonMakeStep(parent);
}

bool PythonMakeStepFactory::canClone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *source) const
{
    const QString id(source->id());
    return canCreate(parent, id);
}

ProjectExplorer::BuildStep *PythonMakeStepFactory::clone(ProjectExplorer::BuildStepList *parent,
                                                          ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    PythonMakeStep *old(qobject_cast<PythonMakeStep *>(source));
    Q_ASSERT(old);
    return new PythonMakeStep(parent, old);
}

bool PythonMakeStepFactory::canRestore(ProjectExplorer::BuildStepList *parent,
                                        const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

ProjectExplorer::BuildStep *PythonMakeStepFactory::restore(ProjectExplorer::BuildStepList *parent,
                                                            const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    PythonMakeStep *bs(new PythonMakeStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QStringList PythonMakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->target()->project()->id() != QLatin1String(Constants::GENERICPROJECT_ID))
        return QStringList();
    return QStringList() << QLatin1String(GENERIC_MS_ID);
}

QString PythonMakeStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(GENERIC_MS_ID))
        return QCoreApplication::translate("PythonProjectManager::Internal::PythonMakeStep",
                                           GENERIC_MS_DISPLAY_NAME);
    return QString();
}
