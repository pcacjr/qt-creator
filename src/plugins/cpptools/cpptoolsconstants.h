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

#ifndef CPPTOOLSCONSTANTS_H
#define CPPTOOLSCONSTANTS_H

#include <QtCore/QtGlobal>

namespace CppTools {
namespace Constants {

const char * const M_TOOLS_CPP              = "CppTools.Tools.Menu";
const char * const SWITCH_HEADER_SOURCE     = "CppTools.SwitchHeaderSource";
const char * const TASK_INDEX               = "CppTools.Task.Index";
const char * const TASK_SEARCH              = "CppTools.Task.Search";
const char * const C_SOURCE_MIMETYPE = "text/x-csrc";
const char * const C_HEADER_MIMETYPE = "text/x-chdr";
const char * const CPP_SOURCE_MIMETYPE = "text/x-c++src";
const char * const OBJECTIVE_CPP_SOURCE_MIMETYPE = "text/x-objcsrc";
const char * const CPP_HEADER_MIMETYPE = "text/x-c++hdr";

// QSettings keys for use by the "New Class" wizards.
const char * const CPPTOOLS_SETTINGSGROUP = "CppTools";
const char * const LOWERCASE_CPPFILES_KEY = "LowerCaseFiles";
enum { lowerCaseFilesDefault = 1 };

const char * const CPP_CODE_STYLE_SETTINGS_ID = "A.Code Style";
const char * const CPP_CODE_STYLE_SETTINGS_NAME = QT_TRANSLATE_NOOP("CppTools", "Code Style");
const char * const CPP_FILE_SETTINGS_ID = "B.File Naming";
const char * const CPP_FILE_SETTINGS_NAME = QT_TRANSLATE_NOOP("CppTools", "File Naming");
const char * const CPP_SETTINGS_CATEGORY = "I.C++";
const char * const CPP_SETTINGS_TR_CATEGORY = QT_TRANSLATE_NOOP("CppTools", "C++");
const char * const SETTINGS_CATEGORY_CPP_ICON = ":/core/images/category_cpp.png";

const char * const CPP_SETTINGS_ID = "Cpp";
const char * const CPP_SETTINGS_NAME = QT_TRANSLATE_NOOP("CppTools", "C++");

} // namespace Constants
} // namespace CppTools

#endif // CPPTOOLSCONSTANTS_H
