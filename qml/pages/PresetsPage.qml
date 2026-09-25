import QtQuick 2.0
import Sailfish.Silica 1.0
import "../display.js" as Display

Page {
    id: page

    // Tapping a preset starts a drink of it instead of editing it
    property bool picking

    allowedOrientations: Orientation.All

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: presetStore

        header: PageHeader {
            title: page.picking ? qsTr("Start drink") : qsTr("Presets")
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Add preset")
                onClicked: pageStack.push(Qt.resolvedUrl("PresetDialog.qml"))
            }
        }

        ViewPlaceholder {
            enabled: presetStore.count === 0
            text: qsTr("No presets")
            hintText: qsTr("Pull down to add one")
        }

        delegate: ListItem {
            id: listItem

            contentHeight: Theme.itemSizeMedium
            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Edit")
                    onClicked: pageStack.push(Qt.resolvedUrl("PresetDialog.qml"), { presetId: model.presetId })
                }
                MenuItem {
                    text: qsTr("Duplicate")
                    onClicked: pageStack.push(Qt.resolvedUrl("PresetDialog.qml"), { copyOf: model.presetId })
                }
                MenuItem {
                    text: qsTr("Remove")
                    onClicked: {
                        var presetId = model.presetId
                        listItem.remorseDelete(function() { presetStore.removePreset(presetId) })
                    }
                }
            }
            onClicked: {
                if (page.picking) {
                    drinkLog.startDrink(model.name, model.volume, model.abv, model.image)
                    pageStack.pop()
                } else {
                    pageStack.push(Qt.resolvedUrl("PresetDialog.qml"), { presetId: model.presetId })
                }
            }

            DrinkPhoto {
                id: photo
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                source: model.image
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                x: photo.visible ? photo.x + photo.width + Theme.paddingMedium : Theme.horizontalPageMargin
                width: parent.width - x - Theme.horizontalPageMargin

                Label {
                    width: parent.width
                    text: model.name
                    truncationMode: TruncationMode.Fade
                    color: listItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    text: Display.details(model.brand, Display.volume(model.volume), Display.abv(model.abv))
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: listItem.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }
        }

        footer: Label {
            visible: presetStore.hasImages
            x: Theme.horizontalPageMargin
            width: listView.width - 2 * x
            height: implicitHeight + 2 * Theme.paddingLarge
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            text: qsTr("Photos from Open Food Facts, CC BY-SA")
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
        }

        VerticalScrollDecorator { }
    }
}
