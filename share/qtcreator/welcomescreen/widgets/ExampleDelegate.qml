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

import QtQuick 1.0

Rectangle {
    id: root
    height: 110
    color: "#00ffffff"
    radius: 6

    Text {
        id: title
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 10
        text: model.name
        font.bold: true
        font.pixelSize: 16;

    }

    RatingBar { id: rating; anchors.top: parent.top; anchors.topMargin: 10; anchors.right: parent.right; anchors.rightMargin: 10; rating: model.difficulty; visible: model.difficulty !== 0 }

    Image {
        property bool hideImage : model.imageUrl === "" || status === Image.Error
        id: image
        anchors.top: title.bottom
        anchors.left: parent.left
        anchors.topMargin: 10
        anchors.leftMargin: 10
        width: hideImage ? 0 : 90
        height: hideImage ? 0 : 66
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: model.imageUrl !== "" ? "image://helpimage/" + encodeURI(model.imageUrl) : ""
    }

    Text {
        id: description
        clip: true
        anchors.left: image.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.top: rating.bottom
        anchors.topMargin: 6
        wrapMode: Text.WordWrap
        text: model.description
    }
    Text { id: labelText; anchors.top: description.bottom; anchors.topMargin: 10; anchors.left: image.right; text: "Tags: "; font.bold: true; }
    Row { id: tagLine; anchors.top: description.bottom; anchors.topMargin: 10; anchors.left: labelText.right; Text { text: model.tags.join(", "); color: "grey" } }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            if (model.hasSourceCode)
                gettingStarted.openProject(model.projectPath, model.filesToOpen, model.docUrl)
            else
                gettingStarted.openSplitHelp(model.docUrl);
        }
        onEntered: parent.state = "hover"
        onExited: parent.state = ""
    }

    states: [ State { name: "hover"; PropertyChanges { target: root; color: "#eeeeeeee" } } ]

    transitions:
        Transition {
        from: ""
        to: "hover"
        reversible: true
        ColorAnimation { duration: 100; easing.type: Easing.OutQuad }
    }
}