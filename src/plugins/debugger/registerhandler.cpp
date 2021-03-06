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

#include "registerhandler.h"
#include "watchdelegatewidgets.h"

//////////////////////////////////////////////////////////////////
//
// RegisterHandler
//
//////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

RegisterHandler::RegisterHandler()
  : m_base(-1)
{
    setNumberBase(16);
}

int RegisterHandler::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_registers.size();
}

int RegisterHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

// Editor value: Preferably number, else string.
QVariant Register::editValue() const
{
    bool ok = true;
    // Try to convert to number?
    const qulonglong v = value.toULongLong(&ok, 0); // Autodetect format
    if (ok)
        return QVariant(v);
    return QVariant(value);
}

// Editor value: Preferably padded number, else padded string.
QString Register::displayValue(int base, int strlen) const
{
    const QVariant editV = editValue();
    if (editV.type() == QVariant::ULongLong)
        return QString::fromAscii("%1").arg(editV.toULongLong(), strlen, base);
    const QString stringValue = editV.toString();
    if (stringValue.size() < strlen)
        return QString(strlen - stringValue.size(), QLatin1Char(' ')) + value;
    return stringValue;
}

QVariant RegisterHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_registers.size())
        return QVariant();

    const Register &reg = m_registers.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0: {
            const QString padding = QLatin1String("  ");
            return QVariant(padding + reg.name + padding);
        }
        case 1: // Display: Pad value for alignment
            return reg.displayValue(m_base, m_strlen);
        } // switch column
    case Qt::EditRole: // Edit: Unpadded for editing
        return reg.editValue();
    case Qt::TextAlignmentRole:
        return index.column() == 1 ? QVariant(Qt::AlignRight) : QVariant();
    default:
        break;
    }
    return QVariant();
}

QVariant RegisterHandler::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Name");
            case 1: return tr("Value (base %1)").arg(m_base);
        };
    }
    return QVariant();
}

Qt::ItemFlags RegisterHandler::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::ItemFlags();

    const Qt::ItemFlags notEditable = Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    // Can edit registers if they are hex numbers and not arrays.
    if (idx.column() == 1
        && IntegerWatchLineEdit::isUnsignedHexNumber(m_registers.at(idx.row()).value))
        return notEditable | Qt::ItemIsEditable;
    return notEditable;
}

void RegisterHandler::removeAll()
{
    m_registers.clear();
    reset();
}

bool RegisterHandler::isEmpty() const
{
    return m_registers.isEmpty();
}

// Compare register sets by name
static inline bool compareRegisterSet(const Registers &r1, const Registers &r2)
{
    if (r1.size() != r2.size())
        return false;
    const int size = r1.size();
    for (int r = 0; r < size; r++)
        if (r1.at(r).name != r2.at(r).name)
            return false;
    return true;
}

void RegisterHandler::setRegisters(const Registers &registers)
{
    m_registers = registers;
    const int size = m_registers.size();
    for (int r = 0; r < size; r++)
        m_registers[r].changed = false;
    calculateWidth();
    reset();
}

void RegisterHandler::setAndMarkRegisters(const Registers &registers)
{
    if (!compareRegisterSet(m_registers, registers)) {
        setRegisters(registers);
        return;
    }
    const int size = m_registers.size();
    for (int r = 0; r < size; r++) {
        const QModelIndex regIndex = index(r, 1);
        if (m_registers.at(r).value != registers.at(r).value) {
            // Indicate red if values change, keep changed.
            m_registers[r].changed = m_registers[r].changed || !m_registers.at(r).value.isEmpty();
            m_registers[r].value = registers.at(r).value;
            emit dataChanged(regIndex, regIndex);
        }
        emit registerSet(regIndex); // notify attached memory views.
    }
}

Registers RegisterHandler::registers() const
{
    return m_registers;
}

void RegisterHandler::calculateWidth()
{
    m_strlen = (m_base == 2 ? 64 : m_base == 8 ? 32 : m_base == 10 ? 26 : 16);
}

void RegisterHandler::setNumberBase(int base)
{
    if (m_base != base) {
        m_base = base;
        calculateWidth();
        emit reset();
    }
}

} // namespace Internal
} // namespace Debugger
