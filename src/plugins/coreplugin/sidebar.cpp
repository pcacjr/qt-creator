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

#include "sidebar.h"
#include "sidebarwidget.h"
#include "imode.h"

#include <coreplugin/coreconstants.h>

#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"

#include <QtCore/QEvent>
#include <QtCore/QSettings>
#include <QtGui/QLayout>
#include <QtGui/QToolBar>
#include <QtGui/QAction>
#include <QtGui/QToolButton>

namespace Core {

SideBarItem::SideBarItem(QWidget *widget, const QString &id) :
    m_id(id), m_widget(widget)
{
}

SideBarItem::~SideBarItem()
{
    delete m_widget;
}

QWidget *SideBarItem::widget() const
{
    return m_widget;
}

QString SideBarItem::id() const
{
    return m_id;
}

QString SideBarItem::title() const
{
    return m_widget->windowTitle();
}

QList<QToolButton *> SideBarItem::createToolBarWidgets()
{
    return QList<QToolButton *>();
}

struct SideBarPrivate {
    SideBarPrivate() :m_closeWhenEmpty(false) {}

    QList<Internal::SideBarWidget*> m_widgets;
    QMap<QString, QWeakPointer<SideBarItem> > m_itemMap;
    QStringList m_availableItemIds;
    QStringList m_availableItemTitles;
    QStringList m_unavailableItemIds;
    QStringList m_defaultVisible;
    QMap<QString, Core::Command*> m_shortcutMap;
    bool m_closeWhenEmpty;
};

SideBar::SideBar(QList<SideBarItem*> itemList,
                 QList<SideBarItem*> defaultVisible) :
    d(new SideBarPrivate)
{
    setOrientation(Qt::Vertical);
    foreach (SideBarItem *item, itemList) {
        d->m_itemMap.insert(item->id(), item);
        d->m_availableItemIds.append(item->id());
        d->m_availableItemTitles.append(item->title());
    }

    foreach (SideBarItem *item, defaultVisible) {
        if (!itemList.contains(item))
            continue;
        d->m_defaultVisible.append(item->id());
    }
}

SideBar::~SideBar()
{
    QMutableMapIterator<QString, QWeakPointer<SideBarItem> > iter(d->m_itemMap);
    while(iter.hasNext()) {
        iter.next();
        if (!iter.value().isNull())
            delete iter.value().data();
    }
}

QString SideBar::idForTitle(const QString &title) const
{
    QMapIterator<QString, QWeakPointer<SideBarItem> > iter(d->m_itemMap);
    while(iter.hasNext()) {
        iter.next();
        if (iter.value().data()->title() == title)
            return iter.key();
    }
    return QString();
}

QStringList SideBar::availableItemIds() const
{
    return d->m_availableItemIds;
}

QStringList SideBar::availableItemTitles() const
{
    return d->m_availableItemTitles;
}

QStringList SideBar::unavailableItemIds() const
{
    return d->m_unavailableItemIds;
}

bool SideBar::closeWhenEmpty() const
{
    return d->m_closeWhenEmpty;
}
void SideBar::setCloseWhenEmpty(bool value)
{
    d->m_closeWhenEmpty = value;
}

void SideBar::makeItemAvailable(SideBarItem *item)
{
    QMap<QString, QWeakPointer<SideBarItem> >::const_iterator it = d->m_itemMap.constBegin();
    while (it != d->m_itemMap.constEnd()) {
        if (it.value().data() == item) {
            d->m_availableItemIds.append(it.key());
            d->m_availableItemTitles.append(it.value().data()->title());
            d->m_unavailableItemIds.removeAll(it.key());
            qSort(d->m_availableItemTitles);
            emit availableItemsChanged();
            //updateWidgets();
            break;
        }
        ++it;
    }
}

// sets a list of externally used, unavailable items. For example,
// another sidebar could set
void SideBar::setUnavailableItemIds(const QStringList &itemIds)
{
    // re-enable previous items
    foreach(const QString &id, d->m_unavailableItemIds) {
        d->m_availableItemIds.append(id);
        d->m_availableItemTitles.append(d->m_itemMap.value(id).data()->title());
    }

    d->m_unavailableItemIds.clear();

    foreach (const QString &id, itemIds) {
        if (!d->m_unavailableItemIds.contains(id))
            d->m_unavailableItemIds.append(id);
        d->m_availableItemIds.removeAll(id);
        d->m_availableItemTitles.removeAll(d->m_itemMap.value(id).data()->title());
    }
    qSort(d->m_availableItemTitles);
    updateWidgets();
}

SideBarItem *SideBar::item(const QString &id)
{
    if (d->m_itemMap.contains(id)) {
        d->m_availableItemIds.removeAll(id);
        d->m_availableItemTitles.removeAll(d->m_itemMap.value(id).data()->title());

        if (!d->m_unavailableItemIds.contains(id))
            d->m_unavailableItemIds.append(id);

        emit availableItemsChanged();
        return d->m_itemMap.value(id).data();
    }
    return 0;
}

Internal::SideBarWidget *SideBar::insertSideBarWidget(int position, const QString &id)
{
    Internal::SideBarWidget *item = new Internal::SideBarWidget(this, id);
    connect(item, SIGNAL(splitMe()), this, SLOT(splitSubWidget()));
    connect(item, SIGNAL(closeMe()), this, SLOT(closeSubWidget()));
    connect(item, SIGNAL(currentWidgetChanged()), this, SLOT(updateWidgets()));
    insertWidget(position, item);
    d->m_widgets.insert(position, item);
    updateWidgets();
    return item;
}

void SideBar::removeSideBarWidget(Internal::SideBarWidget *widget)
{
    widget->removeCurrentItem();
    d->m_widgets.removeOne(widget);
    widget->hide();
    widget->deleteLater();
}

void SideBar::splitSubWidget()
{
    Internal::SideBarWidget *original = qobject_cast<Internal::SideBarWidget*>(sender());
    int pos = indexOf(original) + 1;
    insertSideBarWidget(pos);
    updateWidgets();
}

void SideBar::closeSubWidget()
{
    if (d->m_widgets.count() != 1) {
        Internal::SideBarWidget *widget = qobject_cast<Internal::SideBarWidget*>(sender());
        if (!widget)
            return;
        removeSideBarWidget(widget);
        updateWidgets();
    } else {
        if (d->m_closeWhenEmpty)
            setVisible(false);
    }
}

void SideBar::updateWidgets()
{
    foreach (Internal::SideBarWidget *i, d->m_widgets)
        i->updateAvailableItems();
}

void SideBar::saveSettings(QSettings *settings, const QString &name)
{
    const QString prefix = name.isEmpty() ? name : (name + QLatin1Char('/'));

    QStringList views;
    for (int i = 0; i < d->m_widgets.count(); ++i) {
        QString currentItemId = d->m_widgets.at(i)->currentItemId();
        if (!currentItemId.isEmpty())
            views.append(currentItemId);
    }
    if (views.isEmpty() && d->m_itemMap.size()) {
        QMapIterator<QString, QWeakPointer<SideBarItem> > iter(d->m_itemMap);
        iter.next();
        views.append(iter.key());
    }

    settings->setValue(prefix + "Views", views);
    settings->setValue(prefix + "Visible", true);
    settings->setValue(prefix + "VerticalPosition", saveState());
    settings->setValue(prefix + "Width", width());
}

void SideBar::closeAllWidgets()
{
    foreach (Internal::SideBarWidget *widget, d->m_widgets)
        removeSideBarWidget(widget);
}

void SideBar::readSettings(QSettings *settings, const QString &name)
{
    const QString prefix = name.isEmpty() ? name : (name + QLatin1Char('/'));

    closeAllWidgets();

    if (settings->contains(prefix + "Views")) {
        QStringList views = settings->value(prefix + "Views").toStringList();
        if (views.count()) {
            foreach (const QString &id, views)
                insertSideBarWidget(d->m_widgets.count(), id);

        } else {
            insertSideBarWidget(0);
        }
    } else {
        foreach (const QString &id, d->m_defaultVisible)
            insertSideBarWidget(d->m_widgets.count(), id);
    }

    if (settings->contains(prefix + "Visible"))
        setVisible(settings->value(prefix + "Visible").toBool());

    if (settings->contains(prefix + "VerticalPosition"))
        restoreState(settings->value(prefix + "VerticalPosition").toByteArray());

    if (settings->contains(prefix + "Width")) {
        QSize s = size();
        s.setWidth(settings->value(prefix + "Width").toInt());
        resize(s);
    }
}

void SideBar::activateItem(SideBarItem *item)
{
    QMap<QString, QWeakPointer<SideBarItem> >::const_iterator it = d->m_itemMap.constBegin();
    QString id;
    while (it != d->m_itemMap.constEnd()) {
        if (it.value().data() == item) {
            id = it.key();
            break;
        }
        ++it;
    }

    if (id.isEmpty())
        return;

    for (int i = 0; i < d->m_widgets.count(); ++i) {
        if (d->m_widgets.at(i)->currentItemId() == id) {
            item->widget()->setFocus();
            return;
        }
    }

    Internal::SideBarWidget *widget = d->m_widgets.first();
    widget->setCurrentItem(id);
    updateWidgets();
    item->widget()->setFocus();
}

void SideBar::setShortcutMap(const QMap<QString, Core::Command*> &shortcutMap)
{
    d->m_shortcutMap = shortcutMap;
}

QMap<QString, Core::Command*> SideBar::shortcutMap() const
{
    return d->m_shortcutMap;
}
} // namespace Core

