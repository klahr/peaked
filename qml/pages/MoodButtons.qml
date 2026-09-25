import QtQuick 2.0
import Sailfish.Silica 1.0
import rs.r8.peaked 1.0
import "../display.js" as Display

// Good, OK and Bad side by side
Row {
    id: row

    signal picked(int mood)

    x: Theme.horizontalPageMargin
    spacing: Theme.paddingMedium

    Repeater {
        model: [
            { mood: MoodLog.Good, text: qsTr("Good") },
            { mood: MoodLog.Ok, text: qsTr("OK") },
            { mood: MoodLog.Bad, text: qsTr("Bad") }
        ]

        Button {
            width: (row.parent.width - 2 * Theme.horizontalPageMargin - 2 * row.spacing) / 3
            text: Display.moodEmoji(modelData.mood) + " " + modelData.text
            onClicked: row.picked(modelData.mood)
        }
    }
}
