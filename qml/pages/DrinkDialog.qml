import QtQuick 2.0
import Sailfish.Silica 1.0
import "../display.js" as Display

Dialog {
    id: dialog

    property string drinkId
    property date started
    property int finishHour
    property int finishMinute

    // A finish time before the start time is on the next day
    readonly property date finished: {
        var date = new Date(started.getTime())
        date.setHours(finishHour, finishMinute, 0, 0)
        if (date.getTime() <= started.getTime())
            date.setDate(date.getDate() + 1)
        return date
    }

    allowedOrientations: Orientation.All

    // Accepts either decimal separator whatever the locale
    function parseDecimal(text) {
        return parseFloat(text.replace(",", "."))
    }

    canAccept: nameField.text.trim().length > 0 && volumeField.acceptableInput && abvField.acceptableInput
               && parseDecimal(volumeField.text) > 0 && parseDecimal(abvField.text) > 0
               && parseDecimal(abvField.text) <= 100
               && finished.getTime() <= Date.now()

    onAccepted: drinkLog.updateDrink(drinkId, nameField.text, parseInt(volumeField.text),
                                     parseDecimal(abvField.text), started, finished)

    Component.onCompleted: {
        var drink = drinkLog.drink(drinkId)
        nameField.text = drink.name
        volumeField.text = Math.round(drink.volume)
        abvField.text = drink.abv.toLocaleString(Qt.locale(), "f", drink.abv % 1 === 0 ? 0 : 1)
        var start = new Date(drink.started.getTime())
        start.setSeconds(0, 0)
        started = start
        finishHour = drink.finished.getHours()
        finishMinute = drink.finished.getMinutes()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            DialogHeader {
                title: qsTr("Edit drink")
            }

            TextField {
                id: nameField
                width: parent.width
                label: qsTr("Name")
                placeholderText: label
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

            ValueButton {
                label: qsTr("Started")
                value: Format.formatDate(dialog.started, Formatter.DateMedium)
                onClicked: {
                    var picker = pageStack.push("Sailfish.Silica.DatePickerDialog", { date: dialog.started })
                    picker.accepted.connect(function() {
                        var date = new Date(dialog.started.getTime())
                        date.setFullYear(picker.year, picker.month - 1, picker.day)
                        dialog.started = date
                    })
                }
            }

            ValueButton {
                label: qsTr("Start time")
                value: Display.time(dialog.started)
                onClicked: {
                    var picker = pageStack.push("Sailfish.Silica.TimePickerDialog",
                                                { hour: dialog.started.getHours(), minute: dialog.started.getMinutes() })
                    picker.accepted.connect(function() {
                        var date = new Date(dialog.started.getTime())
                        date.setHours(picker.hour, picker.minute, 0, 0)
                        dialog.started = date
                    })
                }
            }

            ValueButton {
                label: qsTr("Finish time")
                value: dialog.finished.getDate() !== dialog.started.getDate()
                       ? qsTr("%1, next day").arg(Display.time(dialog.finished))
                       : Display.time(dialog.finished)
                onClicked: {
                    var picker = pageStack.push("Sailfish.Silica.TimePickerDialog",
                                                { hour: dialog.finishHour, minute: dialog.finishMinute })
                    picker.accepted.connect(function() {
                        dialog.finishHour = picker.hour
                        dialog.finishMinute = picker.minute
                    })
                }
            }

            Label {
                visible: dialog.finished.getTime() > Date.now()
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                text: qsTr("The drink can not finish in the future")
                color: Theme.errorColor
                font.pixelSize: Theme.fontSizeSmall
            }
        }
    }
}
