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

#include "qmljseditorplugin.h"
#include "qmljshighlighter.h"
#include "qmljseditor.h"
#include "qmljseditorconstants.h"
#include "qmljseditorfactory.h"
#include "qmljshoverhandler.h"
#include "qmlfilewizard.h"
#include "jsfilewizard.h"
#include "qmljsoutline.h"
#include "qmljspreviewrunner.h"
#include "qmljssnippetprovider.h"
#include "qmltaskmanager.h"
#include "quicktoolbar.h"
#include "quicktoolbarsettingspage.h"
#include "qmljscompletionassist.h"
#include "qmljsquickfixassist.h"

#include <qmljs/qmljsicons.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <qmldesigner/qmldesignerconstants.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/taskhub.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textfilewizard.h>
#include <texteditor/texteditoractionhandler.h>
#include <utils/qtcassert.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtGui/QMenu>
#include <QtGui/QAction>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSEditor::Constants;

enum {
    QUICKFIX_INTERVAL = 20
};

void registerQuickFixes(ExtensionSystem::IPlugin *plugIn);

QmlJSEditorPlugin *QmlJSEditorPlugin::m_instance = 0;

QmlJSEditorPlugin::QmlJSEditorPlugin() :
        m_modelManager(0),
    m_wizard(0),
    m_editor(0),
    m_actionHandler(0),
    m_quickFixAssistProvider(0)
{
    m_instance = this;
}

QmlJSEditorPlugin::~QmlJSEditorPlugin()
{
    removeObject(m_editor);
    delete m_actionHandler;
    m_instance = 0;
}

/*! Copied from cppplugin.cpp */
static inline
Core::Command *createSeparator(Core::ActionManager *am,
                               QObject *parent,
                               Core::Context &context,
                               const char *id)
{
    QAction *separator = new QAction(parent);
    separator->setSeparator(true);
    return am->registerAction(separator, Core::Id(id), context);
}

bool QmlJSEditorPlugin::initialize(const QStringList & /*arguments*/, QString *error_message)
{
    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/qmljseditor/QmlJSEditor.mimetypes.xml"), error_message))
        return false;

    m_modelManager = QmlJS::ModelManagerInterface::instance();
    addAutoReleasedObject(new QmlJSSnippetProvider);

    Core::Context context(QmlJSEditor::Constants::C_QMLJSEDITOR_ID);

    m_editor = new QmlJSEditorFactory(this);
    addObject(m_editor);

    Core::BaseFileWizardParameters qmlWizardParameters(Core::IWizard::FileWizard);
    qmlWizardParameters.setCategory(QLatin1String(Constants::WIZARD_CATEGORY_QML));
    qmlWizardParameters.setDisplayCategory(QCoreApplication::translate("QmlJsEditor", Constants::WIZARD_TR_CATEGORY_QML));
    qmlWizardParameters.setDescription(tr("Creates a QML file."));
    qmlWizardParameters.setDisplayName(tr("QML File"));
    qmlWizardParameters.setId(QLatin1String("Q.Qml"));
    addAutoReleasedObject(new QmlFileWizard(qmlWizardParameters, core));

    Core::BaseFileWizardParameters jsWizardParameters(Core::IWizard::FileWizard);
    jsWizardParameters.setCategory(QLatin1String(Constants::WIZARD_CATEGORY_QML));
    jsWizardParameters.setDisplayCategory(QCoreApplication::translate("QmlJsEditor", Constants::WIZARD_TR_CATEGORY_QML));
    jsWizardParameters.setDescription(tr("Creates a JavaScript file."));
    jsWizardParameters.setDisplayName(tr("JS File"));
    jsWizardParameters.setId(QLatin1String("Z.Js"));
    addAutoReleasedObject(new JsFileWizard(jsWizardParameters, core));

    m_actionHandler = new TextEditor::TextEditorActionHandler(QmlJSEditor::Constants::C_QMLJSEDITOR_ID,
          TextEditor::TextEditorActionHandler::Format
        | TextEditor::TextEditorActionHandler::UnCommentSelection
        | TextEditor::TextEditorActionHandler::UnCollapseAll);
    m_actionHandler->initializeActions();

    Core::ActionManager *am =  core->actionManager();
    Core::ActionContainer *contextMenu = am->createMenu(QmlJSEditor::Constants::M_CONTEXT);
    Core::ActionContainer *qmlToolsMenu = am->createMenu(Core::Id(Constants::M_TOOLS_QML));
    qmlToolsMenu->setOnAllDisabledBehavior(Core::ActionContainer::Hide);
    QMenu *menu = qmlToolsMenu->menu();
    //: QML sub-menu in the Tools menu
    menu->setTitle(tr("QML"));
    am->actionContainer(Core::Constants::M_TOOLS)->addMenu(qmlToolsMenu);

    Core::Command *cmd;
    QAction *followSymbolUnderCursorAction = new QAction(tr("Follow Symbol Under Cursor"), this);
    cmd = am->registerAction(followSymbolUnderCursorAction, Constants::FOLLOW_SYMBOL_UNDER_CURSOR, context);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F2));
    connect(followSymbolUnderCursorAction, SIGNAL(triggered()), this, SLOT(followSymbolUnderCursor()));
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    QAction *findUsagesAction = new QAction(tr("Find Usages"), this);
    cmd = am->registerAction(findUsagesAction, Constants::FIND_USAGES, context);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+U")));
    connect(findUsagesAction, SIGNAL(triggered()), this, SLOT(findUsages()));
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    QAction *showQuickToolbar = new QAction(tr("Show Qt Quick Toolbar"), this);
    cmd = am->registerAction(showQuickToolbar, Constants::SHOW_QT_QUICK_HELPER, context);
#ifdef Q_WS_MACX
    cmd->setDefaultKeySequence(QKeySequence(Qt::META + Qt::ALT + Qt::Key_Space));
#else
    cmd->setDefaultKeySequence(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Space));
#endif
    connect(showQuickToolbar, SIGNAL(triggered()), this, SLOT(showContextPane()));
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    // Insert marker for "Refactoring" menu:
    Core::Context globalContext(Core::Constants::C_GLOBAL);
    Core::Command *sep = createSeparator(am, this, globalContext,
                                         Constants::SEPARATOR1);
    sep->action()->setObjectName(Constants::M_REFACTORING_MENU_INSERTION_POINT);
    contextMenu->addAction(sep);
    contextMenu->addAction(createSeparator(am, this, globalContext,
                                           Constants::SEPARATOR2));

    cmd = am->command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    contextMenu->addAction(cmd);

    cmd = am->command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    m_quickFixAssistProvider = new QmlJSQuickFixAssistProvider;
    addAutoReleasedObject(m_quickFixAssistProvider);
    addAutoReleasedObject(new QmlJSCompletionAssistProvider);

    addAutoReleasedObject(new HoverHandler);

    error_message->clear();

    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(":/qmljseditor/images/qmlfile.png")), "qml");

    registerQuickFixes(this);

    addAutoReleasedObject(new QmlJSOutlineWidgetFactory);

    m_qmlTaskManager = new QmlTaskManager;
    addAutoReleasedObject(m_qmlTaskManager);

    connect(m_modelManager, SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)),
            m_qmlTaskManager, SLOT(updateMessages()));
    // recompute messages when information about libraries changes
    connect(m_modelManager, SIGNAL(libraryInfoUpdated(QString,QmlJS::LibraryInfo)),
            m_qmlTaskManager, SLOT(updateMessages()));
    // recompute messages when project data changes (files added or removed)
    connect(m_modelManager, SIGNAL(projectInfoUpdated(ProjectInfo)),
            m_qmlTaskManager, SLOT(updateMessages()));
    connect(m_modelManager, SIGNAL(aboutToRemoveFiles(QStringList)),
            m_qmlTaskManager, SLOT(documentsRemoved(QStringList)));

    addAutoReleasedObject(new QuickToolBar);
    addAutoReleasedObject(new Internal::QuickToolBarSettingsPage);

    connect(core->editorManager(), SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(currentEditorChanged(Core::IEditor*)));

    return true;
}

void QmlJSEditorPlugin::extensionsInitialized()
{
    ProjectExplorer::TaskHub *taskHub =
        ExtensionSystem::PluginManager::instance()->getObject<ProjectExplorer::TaskHub>();
    taskHub->addCategory(Constants::TASK_CATEGORY_QML, tr("QML"));
}

ExtensionSystem::IPlugin::ShutdownFlag QmlJSEditorPlugin::aboutToShutdown()
{
    delete QmlJS::Icons::instance(); // delete object held by singleton

    return IPlugin::aboutToShutdown();
}

void QmlJSEditorPlugin::initializeEditor(QmlJSEditor::QmlJSTextEditorWidget *editor)
{
    QTC_ASSERT(m_instance, /**/);

    m_actionHandler->setupActions(editor);

    editor->setLanguageSettingsId(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);
}

void QmlJSEditorPlugin::followSymbolUnderCursor()
{
    Core::EditorManager *em = Core::EditorManager::instance();

    if (QmlJSTextEditorWidget *editor = qobject_cast<QmlJSTextEditorWidget*>(em->currentEditor()->widget()))
        editor->followSymbolUnderCursor();
}

void QmlJSEditorPlugin::findUsages()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    if (QmlJSTextEditorWidget *editor = qobject_cast<QmlJSTextEditorWidget*>(em->currentEditor()->widget()))
        editor->findUsages();
}

void QmlJSEditorPlugin::showContextPane()
{
    Core::EditorManager *em = Core::EditorManager::instance();

    if (QmlJSTextEditorWidget *editor = qobject_cast<QmlJSTextEditorWidget*>(em->currentEditor()->widget()))
        editor->showContextPane();

}

Core::Command *QmlJSEditorPlugin::addToolAction(QAction *a, Core::ActionManager *am,
                                          Core::Context &context, const QString &name,
                                          Core::ActionContainer *c1, const QString &keySequence)
{
    Core::Command *command = am->registerAction(a, name, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    c1->addAction(command);
    return command;
}

QmlJSQuickFixAssistProvider *QmlJSEditorPlugin::quickFixAssistProvider() const
{
    return m_quickFixAssistProvider;
}

void QmlJSEditorPlugin::currentEditorChanged(Core::IEditor *editor)
{
    if (! editor)
        return;

    else if (QmlJSTextEditorWidget *textEditor = qobject_cast<QmlJSTextEditorWidget *>(editor->widget())) {
        textEditor->forceSemanticRehighlight();
    }
}

Q_EXPORT_PLUGIN(QmlJSEditorPlugin)
