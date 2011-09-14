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

#include "pythonproject.h"

#include "pythonbuildconfiguration.h"
#include "pythonprojectconstants.h"
#include "pythontarget.h"

#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/abi.h>
#include <cplusplus/ModelManagerInterface.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>

#include <QtCore/QDir>
#include <QtCore/QProcessEnvironment>

#include <QtGui/QFormLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QComboBox>

using namespace PythonProjectManager;
using namespace PythonProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
const char * const TOOLCHAIN_KEY("PythonProjectManager.PythonProject.Toolchain");
} // end of anonymous namespace

////////////////////////////////////////////////////////////////////////////////////
// PythonProject
////////////////////////////////////////////////////////////////////////////////////

PythonProject::PythonProject(Manager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_toolChain(0)
{
    setProjectContext(Core::Context(PythonProjectManager::Constants::PROJECTCONTEXT));
    setProjectLanguage(Core::Context(ProjectExplorer::Constants::LANG_CXX));

    QFileInfo fileInfo(m_fileName);
    QDir dir = fileInfo.dir();

    m_projectName      = fileInfo.completeBaseName();
    m_filesFileName    = QFileInfo(dir, m_projectName + QLatin1String(".files")).absoluteFilePath();
    m_includesFileName = QFileInfo(dir, m_projectName + QLatin1String(".includes")).absoluteFilePath();
    m_configFileName   = QFileInfo(dir, m_projectName + QLatin1String(".config")).absoluteFilePath();

    m_file = new PythonProjectFile(this, fileName);
    m_rootNode = new PythonProjectNode(this, m_file);

    m_manager->registerProject(this);
}

PythonProject::~PythonProject()
{
    m_codeModelFuture.cancel();
    m_manager->unregisterProject(this);

    delete m_rootNode;
    // do not delete m_toolChain
}

PythonTarget *PythonProject::activeTarget() const
{
    return static_cast<PythonTarget *>(Project::activeTarget());
}

QString PythonProject::filesFileName() const
{
    return m_filesFileName;
}

QString PythonProject::includesFileName() const
{
    return m_includesFileName;
}

QString PythonProject::configFileName() const
{
    return m_configFileName;
}

static QStringList readLines(const QString &absoluteFileName)
{
    QStringList lines;

    QFile file(absoluteFileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        forever {
            QString line = stream.readLine();
            if (line.isNull())
                break;

            lines.append(line);
        }
    }

    return lines;
}

bool PythonProject::saveRawFileList(const QStringList &rawFileList)
{
    // Make sure we can open the file for writing
    Utils::FileSaver saver(filesFileName(), QIODevice::Text);
    if (!saver.hasError()) {
        QTextStream stream(saver.file());
        foreach (const QString &filePath, rawFileList)
            stream << filePath << QLatin1Char('\n');
        saver.setResult(&stream);
    }
    if (!saver.finalize(Core::ICore::instance()->mainWindow()))
        return false;
    refresh(PythonProject::Files);
    return true;
}

bool PythonProject::addFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    QDir baseDir(QFileInfo(m_fileName).dir());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    return saveRawFileList(newList);
}

bool PythonProject::removeFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    foreach (const QString &filePath, filePaths) {
        QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
        if (i != m_rawListEntries.end())
            newList.removeOne(i.value());
    }

    return saveRawFileList(newList);
}

bool PythonProject::setFiles(const QStringList &filePaths)
{
    QStringList newList;
    QDir baseDir(QFileInfo(m_fileName).dir());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    return saveRawFileList(newList);
}

void PythonProject::parseProject(RefreshOptions options)
{
    if (options & Files) {
        m_rawListEntries.clear();
        m_rawFileList = readLines(filesFileName());
        m_files = processEntries(m_rawFileList, &m_rawListEntries);
    }

    if (options & Configuration) {
        m_projectIncludePaths = processEntries(readLines(includesFileName()));

        // TODO: Possibly load some configuration from the project file
        //QSettings projectInfo(m_fileName, QSettings::IniFormat);

        m_defines.clear();

        QFile configFile(configFileName());
        if (configFile.open(QFile::ReadOnly))
            m_defines = configFile.readAll();
    }

    if (options & Files)
        emit fileListChanged();
}

void PythonProject::refresh(RefreshOptions options)
{
    QSet<QString> oldFileList;
    if (!(options & Configuration))
        oldFileList = m_files.toSet();

    parseProject(options);

    if (options & Files)
        m_rootNode->refresh();

    CPlusPlus::CppModelManagerInterface *modelManager =
        CPlusPlus::CppModelManagerInterface::instance();

    if (modelManager) {
        CPlusPlus::CppModelManagerInterface::ProjectInfo pinfo = modelManager->projectInfo(this);

        if (m_toolChain) {
            pinfo.defines = m_toolChain->predefinedMacros();
            pinfo.defines += '\n';

            foreach (const HeaderPath &headerPath, m_toolChain->systemHeaderPaths()) {
                if (headerPath.kind() == HeaderPath::FrameworkHeaderPath)
                    pinfo.frameworkPaths.append(headerPath.path());
                else
                    pinfo.includePaths.append(headerPath.path());
            }
        }

        pinfo.includePaths += allIncludePaths();
        pinfo.defines += m_defines;

        // ### add _defines.
        pinfo.sourceFiles = files();
        pinfo.sourceFiles += generated();

        QStringList filesToUpdate;

        if (options & Configuration) {
            filesToUpdate = pinfo.sourceFiles;
            filesToUpdate.append(QLatin1String("<configuration>")); // XXX don't hardcode configuration file name
            // Full update, if there's a code model update, cancel it
            m_codeModelFuture.cancel();
        } else if (options & Files) {
            // Only update files that got added to the list
            QSet<QString> newFileList = m_files.toSet();
            newFileList.subtract(oldFileList);
            filesToUpdate.append(newFileList.toList());
        }

        modelManager->updateProjectInfo(pinfo);
        m_codeModelFuture = modelManager->updateSourceFiles(filesToUpdate);
    }
}

/**
 * Expands environment variables in the given \a string when they are written
 * like $$(VARIABLE).
 */
static void expandEnvironmentVariables(const QProcessEnvironment &env, QString &string)
{
    const static QRegExp candidate(QLatin1String("\\$\\$\\((.+)\\)"));

    int index = candidate.indexIn(string);
    while (index != -1) {
        const QString value = env.value(candidate.cap(1));

        string.replace(index, candidate.matchedLength(), value);
        index += value.length();

        index = candidate.indexIn(string, index);
    }
}

/**
 * Expands environment variables and converts the path from relative to the
 * project to an absolute path.
 *
 * The \a map variable is an optional argument that will map the returned
 * absolute paths back to their original \a entries.
 */
QStringList PythonProject::processEntries(const QStringList &paths,
                                           QHash<QString, QString> *map) const
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QDir projectDir(QFileInfo(m_fileName).dir());

    QStringList absolutePaths;
    foreach (const QString &path, paths) {
        QString trimmedPath = path.trimmed();
        if (trimmedPath.isEmpty())
            continue;

        expandEnvironmentVariables(env, trimmedPath);

        const QString absPath = QFileInfo(projectDir, trimmedPath).absoluteFilePath();
        absolutePaths.append(absPath);
        if (map)
            map->insert(absPath, trimmedPath);
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

QStringList PythonProject::allIncludePaths() const
{
    QStringList paths;
    paths += m_includePaths;
    paths += m_projectIncludePaths;
    paths.removeDuplicates();
    return paths;
}

QStringList PythonProject::projectIncludePaths() const
{
    return m_projectIncludePaths;
}

QStringList PythonProject::files() const
{
    return m_files;
}

QStringList PythonProject::generated() const
{
    return m_generated;
}

QStringList PythonProject::includePaths() const
{
    return m_includePaths;
}

void PythonProject::setIncludePaths(const QStringList &includePaths)
{
    m_includePaths = includePaths;
}

QByteArray PythonProject::defines() const
{
    return m_defines;
}

void PythonProject::setToolChain(ToolChain *tc)
{
    if (m_toolChain == tc)
        return;

    m_toolChain = tc;
    refresh(Configuration);

    foreach (Target *t, targets()) {
        foreach (BuildConfiguration *bc, t->buildConfigurations())
            bc->setToolChain(tc);
    }

    emit toolChainChanged(m_toolChain);
}

ToolChain *PythonProject::toolChain() const
{
    return m_toolChain;
}

QString PythonProject::displayName() const
{
    return m_projectName;
}

QString PythonProject::id() const
{
    return QLatin1String(Constants::GENERICPROJECT_ID);
}

Core::IFile *PythonProject::file() const
{
    return m_file;
}

IProjectManager *PythonProject::projectManager() const
{
    return m_manager;
}

QList<Project *> PythonProject::dependsOn()
{
    return QList<Project *>();
}

QList<BuildConfigWidget*> PythonProject::subConfigWidgets()
{
    QList<BuildConfigWidget*> list;
    list << new BuildEnvironmentWidget;
    return list;
}

PythonProjectNode *PythonProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList PythonProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode)
    return m_files; // ### TODO: handle generated files here.
}

QStringList PythonProject::buildTargets() const
{
    QStringList targets;
    targets.append(QLatin1String("all"));
    targets.append(QLatin1String("clean"));
    return targets;
}

QVariantMap PythonProject::toMap() const
{
    QVariantMap map(Project::toMap());
    map.insert(QLatin1String(TOOLCHAIN_KEY), m_toolChain ? m_toolChain->id() : QString());
    return map;
}

bool PythonProject::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    // Sanity check: We need both a buildconfiguration and a runconfiguration!
    QList<Target *> targetList = targets();
    foreach (Target *t, targetList) {
        if (!t->activeBuildConfiguration()) {
            removeTarget(t);
            delete t;
            continue;
        }
        if (!t->activeRunConfiguration())
            t->addRunConfiguration(new CustomExecutableRunConfiguration(t));

        qDebug() << "here\n";
    }

    // Add default setup:
    if (targets().isEmpty()) {
        PythonTargetFactory *factory =
                ExtensionSystem::PluginManager::instance()->getObject<PythonTargetFactory>();
        addTarget(factory->create(this, QLatin1String(GENERIC_DESKTOP_TARGET_ID)));
    }

    QString id = map.value(QLatin1String(TOOLCHAIN_KEY)).toString();
    const ToolChainManager *toolChainManager = ToolChainManager::instance();

    if (!id.isNull()) {
        setToolChain(toolChainManager->findToolChain(id));
    } else {
        ProjectExplorer::Abi abi = ProjectExplorer::Abi::hostAbi();
        abi = ProjectExplorer::Abi(abi.architecture(), abi.os(),  ProjectExplorer::Abi::UnknownFlavor,
                                   abi.binaryFormat(), abi.wordWidth() == 32 ? 32 : 0);
        QList<ToolChain *> tcs = toolChainManager->findToolChains(abi);
        if (tcs.isEmpty())
            tcs = toolChainManager->toolChains();
        if (!tcs.isEmpty())
            setToolChain(tcs.at(0));
    }

    setIncludePaths(allIncludePaths());

    refresh(Everything);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////
// PythonBuildSettingsWidget
////////////////////////////////////////////////////////////////////////////////////

PythonBuildSettingsWidget::PythonBuildSettingsWidget(PythonTarget *target)
    : m_target(target), m_toolChainChooser(0), m_buildConfiguration(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // build directory
    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setEnabled(true);
    m_pathChooser->setBaseDirectory(m_target->pythonProject()->projectDirectory());
    fl->addRow(tr("Build directory:"), m_pathChooser);
    connect(m_pathChooser, SIGNAL(changed(QString)), this, SLOT(buildDirectoryChanged()));

    // tool chain
    m_toolChainChooser = new QComboBox;
    m_toolChainChooser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    updateToolChainList();

    fl->addRow(tr("Tool chain:"), m_toolChainChooser);
    connect(m_toolChainChooser, SIGNAL(activated(int)), this, SLOT(toolChainSelected(int)));
    connect(m_target->pythonProject(), SIGNAL(toolChainChanged(ProjectExplorer::ToolChain*)),
            this, SLOT(toolChainChanged(ProjectExplorer::ToolChain*)));
    connect(ProjectExplorer::ToolChainManager::instance(), SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SLOT(updateToolChainList()));
    connect(ProjectExplorer::ToolChainManager::instance(), SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(updateToolChainList()));
}

PythonBuildSettingsWidget::~PythonBuildSettingsWidget()
{ }

QString PythonBuildSettingsWidget::displayName() const
{ return tr("Python Manager"); }

void PythonBuildSettingsWidget::init(BuildConfiguration *bc)
{
    m_buildConfiguration = static_cast<PythonBuildConfiguration *>(bc);
    m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory());
}

void PythonBuildSettingsWidget::buildDirectoryChanged()
{
    m_buildConfiguration->setBuildDirectory(m_pathChooser->rawPath());
}

void PythonBuildSettingsWidget::toolChainSelected(int index)
{
    using namespace ProjectExplorer;

    ToolChain *tc = static_cast<ToolChain *>(m_toolChainChooser->itemData(index).value<void *>());
    m_target->pythonProject()->setToolChain(tc);
}

void PythonBuildSettingsWidget::toolChainChanged(ProjectExplorer::ToolChain *tc)
{
    for (int i = 0; i < m_toolChainChooser->count(); ++i) {
        ToolChain * currentTc = static_cast<ToolChain *>(m_toolChainChooser->itemData(i).value<void *>());
        if (currentTc != tc)
            continue;
        m_toolChainChooser->setCurrentIndex(i);
        return;
    }
}

void PythonBuildSettingsWidget::updateToolChainList()
{
    m_toolChainChooser->clear();

    QList<ToolChain *> tcs = ToolChainManager::instance()->toolChains();
    if (!m_target->pythonProject()->toolChain()) {
        m_toolChainChooser->addItem(tr("<Invalid tool chain>"), qVariantFromValue(static_cast<void *>(0)));
        m_toolChainChooser->setCurrentIndex(0);
    }
    foreach (ToolChain *tc, tcs) {
        m_toolChainChooser->addItem(tc->displayName(), qVariantFromValue(static_cast<void *>(tc)));
        if (m_target->pythonProject()->toolChain()
                && m_target->pythonProject()->toolChain()->id() == tc->id())
            m_toolChainChooser->setCurrentIndex(m_toolChainChooser->count() - 1);
    }
}

////////////////////////////////////////////////////////////////////////////////////
// PythonProjectFile
////////////////////////////////////////////////////////////////////////////////////

PythonProjectFile::PythonProjectFile(PythonProject *parent, QString fileName)
    : Core::IFile(parent),
      m_project(parent),
      m_fileName(fileName)
{ }

PythonProjectFile::~PythonProjectFile()
{ }

bool PythonProjectFile::save(QString *, const QString &, bool)
{
    return false;
}

QString PythonProjectFile::fileName() const
{
    return m_fileName;
}

QString PythonProjectFile::defaultPath() const
{
    return QString();
}

QString PythonProjectFile::suggestedFileName() const
{
    return QString();
}

QString PythonProjectFile::mimeType() const
{
    return Constants::GENERICMIMETYPE;
}

bool PythonProjectFile::isModified() const
{
    return false;
}

bool PythonProjectFile::isReadOnly() const
{
    return true;
}

bool PythonProjectFile::isSaveAsAllowed() const
{
    return false;
}

void PythonProjectFile::rename(const QString &newName)
{
    // Can't happen
    Q_UNUSED(newName);
    QTC_ASSERT(false, /**/);
}

Core::IFile::ReloadBehavior PythonProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool PythonProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}
