/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MSVCTOOLCHAIN_H
#define MSVCTOOLCHAIN_H

#include "toolchain.h"
#include "toolchainconfigwidget.h"

#include <utils/environment.h>

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// MsvcToolChain
// --------------------------------------------------------------------------

class MsvcToolChain : public ToolChain
{
public:
    enum Type { WindowsSDK, VS };
    enum Platform { s32, s64, ia64, amd64 };

    MsvcToolChain(Type type, const QString &name, Platform platform, const QString &varsBat,
                  const QString &varsBatArg, bool autodetect = false);

    QString typeName() const;
    Abi targetAbi() const;

    bool isValid() const;

    QByteArray predefinedMacros() const;
    QList<HeaderPath> systemHeaderPaths() const;
    void addToEnvironment(Utils::Environment &env) const;
    QString makeCommand() const;
    void setDebuggerCommand(const QString &d);
    virtual QString debuggerCommand() const;
    IOutputParser *outputParser() const;

    virtual QVariantMap toMap() const;
    virtual bool fromMap(const QVariantMap &data);

    ToolChainConfigWidget *configurationWidget();

    bool canClone() const;
    ToolChain *clone() const;

    static QString autoDetectCdbDebugger(QStringList *checkedDirectories = 0);

private:
    QString m_varsBat; // Script to setup environment
    QString m_varsBatArg; // Argument
    QString m_debuggerCommand;
    mutable QByteArray m_predefinedMacros;
    mutable Utils::Environment m_lastEnvironment;
    mutable Utils::Environment m_resultEnvironment;
    mutable QList<HeaderPath> m_headerPathes;
    bool m_is64bit;
    Abi::Architecture m_architecture;
};

// --------------------------------------------------------------------------
// MsvcToolChainFactory
// --------------------------------------------------------------------------

class MsvcToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    QString displayName() const;
    QString id() const;

    QList<ToolChain *> autoDetect();

    ToolChainConfigWidget *configurationWidget(ToolChain *);
};

// --------------------------------------------------------------------------
// MsvcToolChainConfigWidget
// --------------------------------------------------------------------------

class MsvcToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    MsvcToolChainConfigWidget(ToolChain *);

    void apply();
    void discard() { setFromToolChain(); }
    bool isDirty() const;

private slots:
    void autoDetectDebugger();

private:
    void setFromToolChain();
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // MSVCTOOLCHAIN_H