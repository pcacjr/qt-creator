/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
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

#include "bazaarsettings.h"
#include "constants.h"

#include <QtCore/QSettings>

namespace Bazaar {
namespace Internal {

const QLatin1String diffIgnoreWhiteSpaceKey("diffIgnoreWhiteSpace");
const QLatin1String diffIgnoreBlankLinesKey("diffIgnoreBlankLines");

BazaarSettings::BazaarSettings() :
    diffIgnoreWhiteSpace(false),
    diffIgnoreBlankLines(false)
{
    setBinary(QLatin1String(Constants::BAZAARDEFAULT));
}

BazaarSettings& BazaarSettings::operator=(const BazaarSettings& other)
{
    VCSBase::VCSBaseClientSettings::operator=(other);
    if (this != &other) {
        diffIgnoreWhiteSpace = other.diffIgnoreWhiteSpace;
        diffIgnoreBlankLines = other.diffIgnoreBlankLines;
    }
    return *this;
}

bool BazaarSettings::sameUserId(const BazaarSettings& other) const
{
    return userName() == other.userName() && email() == other.email();
}

void BazaarSettings::writeSettings(QSettings *settings, const QString &group) const
{
    VCSBaseClientSettings::writeSettings(settings, group);
    settings->beginGroup(group);
    settings->setValue(diffIgnoreWhiteSpaceKey, diffIgnoreWhiteSpace);
    settings->setValue(diffIgnoreBlankLinesKey, diffIgnoreBlankLines);
    settings->endGroup();
}

void BazaarSettings::readSettings(const QSettings *settings, const QString &group)
{
    VCSBaseClientSettings::readSettings(settings, group);
    const QString keyRoot = group + QLatin1Char('/');
    diffIgnoreWhiteSpace = settings->value(keyRoot + diffIgnoreWhiteSpaceKey, false).toBool();
    diffIgnoreBlankLines = settings->value(keyRoot + diffIgnoreBlankLinesKey, false).toBool();
}

bool BazaarSettings::equals(const VCSBaseClientSettings &rhs) const
{
    const BazaarSettings *bzrRhs = dynamic_cast<const BazaarSettings *>(&rhs);
    if (bzrRhs == 0)
        return false;
    return VCSBaseClientSettings::equals(rhs)
            && diffIgnoreWhiteSpace == bzrRhs->diffIgnoreWhiteSpace
            && diffIgnoreBlankLines == bzrRhs->diffIgnoreBlankLines;
}

} // namespace Internal
} // namespace Bazaar
