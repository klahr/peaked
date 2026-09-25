import QtQuick 2.0
import Sailfish.Silica 1.0
import rs.r8.peeked 1.0

Dialog {
    allowedOrientations: Orientation.All

    // Accepts either decimal separator whatever the locale
    function parseDecimal(text) {
        return parseFloat(text.replace(",", "."))
    }

    canAccept: heightField.acceptableInput && weightField.acceptableInput && ageField.acceptableInput
               && limitField.acceptableInput

    onAccepted: profile.save(parseInt(heightField.text), parseInt(weightField.text), parseInt(ageField.text),
                             sexComboBox.currentIndex === 1 ? Profile.Female : Profile.Male,
                             parseDecimal(limitField.text))

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            DialogHeader {
                title: qsTr("Settings")
            }

            TextField {
                id: heightField
                width: parent.width
                label: qsTr("Height in cm")
                placeholderText: label
                text: profile.height > 0 ? profile.height : ""
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 100; top: 250 }
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: weightField.focus = true
            }

            TextField {
                id: weightField
                width: parent.width
                label: qsTr("Weight in kg")
                placeholderText: label
                text: profile.weight > 0 ? profile.weight : ""
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 30; top: 300 }
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: ageField.focus = true
            }

            TextField {
                id: ageField
                width: parent.width
                label: qsTr("Age in years")
                placeholderText: label
                text: profile.age > 0 ? profile.age : ""
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 10; top: 120 }
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }

            ComboBox {
                id: sexComboBox
                width: parent.width
                label: qsTr("Sex")
                currentIndex: profile.sex === Profile.Female ? 1 : 0
                menu: ContextMenu {
                    MenuItem { text: qsTr("Male") }
                    MenuItem { text: qsTr("Female") }
                }
            }

            TextField {
                id: limitField
                width: parent.width
                label: qsTr("Limit line in ‰")
                placeholderText: label
                text: profile.limit.toLocaleString(Qt.locale(), "f", 2)
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                validator: RegExpValidator { regExp: /^\d([.,]\d{1,2})?$/ }
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                text: qsTr("Blood alcohol is only an estimate. Never use it to decide whether you can drive.")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
            }
        }
    }
}
