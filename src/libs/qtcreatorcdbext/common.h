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

#ifndef COMMON_H
#define COMMON_H

// Define KDEXT_64BIT to make all wdbgexts APIs recognize 64 bit addresses
// It is recommended for extensions to use 64 bit headers from wdbgexts so
// the extensions could support 64 bit targets.

#include <string>
#include <sstream>

#include <windows.h>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include <dbgeng.h>

static const char creatorOutputPrefixC[] = "QtCreatorExt: ";

typedef IDebugControl CIDebugControl;
typedef IDebugSymbols3 CIDebugSymbols;
typedef IDebugSymbolGroup2 CIDebugSymbolGroup;
typedef IDebugClient5 CIDebugClient;
typedef IDebugSystemObjects CIDebugSystemObjects;
typedef IDebugDataSpaces4 CIDebugDataSpaces;
typedef IDebugAdvanced2 CIDebugAdvanced;
typedef IDebugRegisters2 CIDebugRegisters;

// Utility messages
std::string winErrorMessage(unsigned long error);
std::string winErrorMessage();
std::string msgDebugEngineComResult(HRESULT hr);
std::string msgDebugEngineComFailed(const char *func, HRESULT hr);

// Debug helper for anything streamable as in 'DebugPrint() << object'
// Derives from std::ostringstream and print out everything accumulated in destructor.
struct DebugPrint : public std::ostringstream {
    DebugPrint() {}
    ~DebugPrint() {
        dprintf("DEBUG: %s\n", str().c_str());
    }
};

ULONG currentThreadId(IDebugSystemObjects *sysObjects);
ULONG currentThreadId(CIDebugClient *client);
ULONG currentProcessId(IDebugSystemObjects *sysObjects);
ULONG currentProcessId(CIDebugClient *client);

#endif // COMMON_H