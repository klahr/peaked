import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

Dialog {
    id: dialog

    // Empty for a new preset, otherwise the preset being edited
    property string presetId
    // A local file, or a URL from the search that is downloaded when saving
    property string image

    allowedOrientations: Orientation.All

    // Accepts either decimal separator whatever the locale
    function parseDecimal(text) {
        return parseFloat(text.replace(",", "."))
    }

    canAccept: nameField.text.trim().length > 0 && volumeField.acceptableInput && abvField.acceptableInput
               && parseDecimal(volumeField.text) > 0 && parseDecimal(abvField.text) > 0
               && parseDecimal(abvField.text) <= 100

    onAccepted: presetStore.savePreset(presetId, nameField.text, brandField.text,
                                       parseInt(volumeField.text), parseDecimal(abvField.text), image)

    function formatDecimal(value, decimals) {
        return value.toLocaleString(Qt.locale(), "f", decimals)
    }

    Component.onCompleted: {
        if (presetId.length === 0)
            return
        var preset = presetStore.preset(presetId)
        image = preset.image
        nameField.text = preset.name
        brandField.text = preset.brand
        volumeField.text = Math.round(preset.volume)
        abvField.text = preset.abv.toLocaleString(Qt.locale(), "f", preset.abv % 1 === 0 ? 0 : 1)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            DialogHeader {
                title: presetId.length > 0 ? qsTr("Edit preset") : qsTr("New preset")
            }

            Component {
                id: imagePickerPage

                ImagePickerPage {
                    onSelectedContentChanged: dialog.image = selectedContent.toString()
                }
            }

            ButtonLayout {
                Button {
                    text: qsTr("Choose photo")
                    onClicked: pageStack.push(imagePickerPage)
                }
                Button {
                    text: qsTr("Take photo")
                    onClicked: {
                        var cameraPage = pageStack.push(Qt.resolvedUrl("CameraPage.qml"))
                        cameraPage.captured.connect(function(url) { dialog.image = url })
                    }
                }
                Button {
                    text: qsTr("Search online")
                    onClicked: {
                        var searchPage = pageStack.push(Qt.resolvedUrl("ProductSearchPage.qml"))
                        searchPage.picked.connect(function(name, brand, volume, abv, pickedImage) {
                            image = pickedImage
                            nameField.text = name
                            brandField.text = brand
                            volumeField.text = volume > 0 ? Math.round(volume) : ""
                            abvField.text = abv > 0 ? formatDecimal(abv, abv % 1 === 0 ? 0 : 1) : ""
                        })
                    }
                }
            }

            Row {
                visible: image.length > 0
                x: Theme.horizontalPageMargin
                spacing: Theme.paddingLarge

                DrinkPhoto {
                    width: Theme.itemSizeExtraLarge
                    height: width
                    source: image
                }
                Button {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Remove photo")
                    onClicked: image = ""
                }
            }

            TextField {
                id: nameField
                width: parent.width
                label: qsTr("Name")
                placeholderText: label
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: brandField.focus = true
            }

            TextField {
                id: brandField
                width: parent.width
                label: qsTr("Brand, optional")
                placeholderText: qsTr("Brand")
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: volumeField.focus = true
            }

            TextField {
                id: volumeField
                width: parent.width
                label: qsTr("Size in ml")
                placeholderText: label
                inputMethodHints: Qt.ImhDigitsOnly
                validator: RegExpValidator { regExp: /^\d{1,4}$/ }
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: abvField.focus = true
            }

            TextField {
                id: abvField
                width: parent.width
                label: qsTr("Alcohol in %")
                placeholderText: label
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                validator: RegExpValidator { regExp: /^\d{1,3}([.,]\d)?$/ }
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
        }
    }
}
