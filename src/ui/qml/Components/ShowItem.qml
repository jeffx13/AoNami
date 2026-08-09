import QtQuick
import ".."

Item {
    id: showItem

    property alias showTitle: titleText.text
    property alias showCover: coverImage.source
    property alias image: imageClip
    property real  aspectRatio: 319 / 225
    property int   libraryType: -1
    property string badgeText: ""

    signal imageClicked(var mouse)
    signal imageLoaded(real sourceAspectRatio)
    signal playClicked()
    signal addClicked()

    // Hide the Add (+) quick action where it doesn't apply (e.g. Library).
    property bool showAddAction: true

    readonly property bool isHovered: imageArea.containsMouse || titleArea.containsMouse
                                      || quickActions.hovered

    Rectangle {
        id: card
        anchors.fill: parent
        color: "transparent"
        radius: 10
        border.color: showItem.isHovered ? Theme.accent : Theme.border
        border.width: showItem.isHovered ? 2 : 1
        Behavior on border.color { ColorAnimation { duration: 200 } }
        Behavior on border.width { NumberAnimation { duration: 200 } }

        Rectangle {
            id: imageClip
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: 2
            }
            height: width * showItem.aspectRatio
            radius: 8
            clip: true
            color: Theme.surface

            Image {
                id: coverImage
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                cache: true
                asynchronous: true
                // Fixed decode size so resizing the cell (e.g. sidebar expanding) just scales
                // the cached texture instead of re-decoding the source.
                sourceSize: Qt.size(360, Math.round(360 * showItem.aspectRatio))
                scale: showItem.isHovered ? 1.06 : 1.0
                opacity: status === Image.Ready ? 1.0 : 0.0
                Behavior on scale {
                    NumberAnimation { duration: 350; easing.type: Easing.OutCubic }
                }
                Behavior on opacity {
                    NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
                }
                // Separate placeholder for errors - reassigning source would stick across reuse.
                onStatusChanged: {
                    if (status === Image.Ready && sourceSize.width > 0 && sourceSize.height > 0)
                        showItem.imageLoaded(sourceSize.height / sourceSize.width)
                }
            }

            Image {
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                visible: coverImage.status === Image.Error
                source: visible ? "qrc:/AoNami/resources/images/error_image.png" : ""
            }

            Rectangle {
                id: skeleton
                anchors.fill: parent
                visible: coverImage.status !== Image.Ready
                color: Theme.surfaceAlt
                clip: true

                Rectangle {
                    width: skeleton.width * 0.6
                    height: skeleton.height
                    rotation: 12
                    transformOrigin: Item.Center
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 0.5; color: Qt.alpha(Theme.textPrimary, 0.10) }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                    NumberAnimation on x {
                        from: -skeleton.width * 0.6
                        to: skeleton.width
                        duration: 1150
                        loops: Animation.Infinite
                        running: skeleton.visible
                    }
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                opacity: showItem.isHovered ? 1.0 : 0.0
                Behavior on opacity {
                    NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
                }
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 0.6; color: "transparent" }
                    GradientStop { position: 1.0; color: "#C0000000" }
                }
            }

            Rectangle {
                width: parent.width * 0.7
                height: parent.height * 0.4
                radius: parent.radius
                anchors {
                    top: parent.top
                    left: parent.left
                }
                opacity: showItem.isHovered ? 0.08 : 0.0
                Behavior on opacity {
                    NumberAnimation { duration: 400; easing.type: Easing.OutCubic }
                }
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#ffffff" }
                    GradientStop { position: 1.0; color: "transparent" }
                }
            }

            Rectangle {
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                    leftMargin: parent.width * 0.15
                    rightMargin: parent.width * 0.15
                }
                height: 2
                radius: 1
                color: Theme.accent
                opacity: showItem.isHovered ? 0.8 : 0.0
                Behavior on opacity {
                    NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
                }
            }

            Rectangle {
                id: badgeGlow
                visible: showItem.libraryType >= 0
                anchors {
                    top: parent.top
                    right: parent.right
                    topMargin: 3
                    rightMargin: 3
                }
                width: libraryBadge.width + 14
                height: libraryBadge.height + 10
                radius: libraryBadge.radius + 5
                color: libraryBadge.typeColor
                opacity: showItem.isHovered ? 0.38 : 0.20
                Behavior on opacity { NumberAnimation { duration: 250 } }
            }

            Rectangle {
                id: libraryBadge
                visible: showItem.libraryType >= 0

                readonly property color typeColor: {
                    switch (showItem.libraryType) {
                        case 0: return "#06B6D4"   // Watching  - cyan
                        case 1: return "#8B5CF6"   // Planned   - violet
                        case 2: return "#F59E0B"   // Paused    - amber
                        case 3: return Theme.danger   // Dropped   - red
                        case 4: return Theme.success   // Completed - emerald
                        default: return Theme.accent
                    }
                }

                anchors {
                    top: parent.top
                    right: parent.right
                    topMargin: 8
                    rightMargin: 8
                }

                implicitWidth: badgeRow.implicitWidth + 18
                height: 26
                radius: 13
                color: Theme.surface
                border.color: typeColor
                border.width: 1

                scale: showItem.isHovered ? 1.08 : 1.0
                Behavior on scale { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }

                Row {
                    id: badgeRow
                    anchors.centerIn: parent
                    spacing: 5

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: libraryBadge.typeColor
                        anchors.verticalCenter: parent.verticalCenter

                        Rectangle {
                            anchors.centerIn: parent
                            width: 4
                            height: 4
                            radius: 2
                            color: "white"
                            opacity: 0.55
                        }
                    }

                    Text {
                        text: showItem.libraryType >= 0 ? Globals.libraryTypes[showItem.libraryType] : ""
                        color: Theme.textPrimary
                        font.pixelSize: Globals.sp(20)
                        font.weight: Font.Medium
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            Rectangle {
                visible: showItem.badgeText.length > 0 && opacity > 0.01
                // Yields to the add button, which shares this corner on hover.
                opacity: showItem.showAddAction && showItem.isHovered ? 0.0 : 1.0
                Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    bottomMargin: 8
                    leftMargin: 8
                }
                implicitWidth: badgeLabel.implicitWidth + 14
                height: 22
                radius: 11
                color: Theme.surface
                border.color: Theme.accent
                border.width: 1

                Text {
                    id: badgeLabel
                    anchors.centerIn: parent
                    text: showItem.badgeText
                    color: Theme.textPrimary
                    font.pixelSize: Globals.sp(18)
                    font.weight: Font.Medium
                }
            }

            MouseArea {
                id: imageArea
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                cursorShape: Qt.PointingHandCursor
                onClicked: (mouse) => showItem.imageClicked(mouse)
            }

            // Netflix-style quick actions, revealed on hover.
            Item {
                id: quickActions
                anchors.fill: parent
                property bool hovered: playBtnArea.containsMouse || addBtnArea.containsMouse
                opacity: showItem.isHovered ? 1.0 : 0.0
                visible: opacity > 0.01
                Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

                Rectangle {
                    id: playBtn
                    anchors { right: parent.right; bottom: parent.bottom; rightMargin: 8; bottomMargin: 8 }
                    width: 40
                    height: 40
                    radius: 20
                    color: playBtnArea.containsMouse ? Theme.accentLight : Theme.accent
                    scale: showItem.isHovered ? 1.0 : 0.7
                    Behavior on scale { NumberAnimation { duration: 220; easing.type: Easing.OutBack } }

                    AppIcon {
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: 1
                        name: "play"
                        size: 18
                        color: "white"
                    }

                    MouseArea {
                        id: playBtnArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: showItem.playClicked()
                    }
                }

                Rectangle {
                    id: addBtn
                    visible: showItem.showAddAction
                    anchors { left: parent.left; bottom: parent.bottom; leftMargin: 8; bottomMargin: 8 }
                    width: 32
                    height: 32
                    radius: 16
                    color: addBtnArea.containsMouse ? Theme.accent : Theme.surface
                    border.color: Theme.accent
                    border.width: 1
                    scale: showItem.isHovered ? 1.0 : 0.7
                    Behavior on scale { NumberAnimation { duration: 220; easing.type: Easing.OutBack } }

                    AppIcon {
                        anchors.centerIn: parent
                        name: "plus"
                        size: 18
                        color: addBtnArea.containsMouse ? "white" : Theme.textPrimary
                    }

                    MouseArea {
                        id: addBtnArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: showItem.addClicked()
                    }
                }
            }
        }

        Text {
            id: titleText
            anchors {
                top: imageClip.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                topMargin: 6
                leftMargin: 6
                rightMargin: 6
                bottomMargin: 4
            }
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignTop
            font.pixelSize: Globals.sp(20)
            color: showItem.isHovered ? Theme.textPrimary : Theme.textSecondary
            Behavior on color {
                ColorAnimation { duration: 200 }
            }
            transform: Translate {
                y: showItem.isHovered ? -2 : 0
                Behavior on y {
                    NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
                }
            }
            clip: true

            MouseArea {
                id: titleArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                onClicked: (mouse) => showItem.imageClicked(mouse)
            }
        }
    }
}