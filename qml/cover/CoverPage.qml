import QtQuick 2.0
import Sailfish.Silica 1.0
import "../display.js" as Display
import rs.r8.peaked 1.0

CoverBackground {
    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingLarge
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Peaked")
            color: Theme.secondaryColor
        }
        Label {
            visible: profile.configured
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: Display.perMille(bloodAlcohol.current)
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeLarge
        }
        Label {
            visible: profile.configured && !isNaN(bloodAlcohol.soberAt.getTime())
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("Sober in %1").arg(Display.duration(bloodAlcohol.soberAt.getTime() - bloodAlcohol.now.getTime()))
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryHighlightColor
        }
        Label {
            visible: profile.configured && (bloodAlcohol.current > 0 || drinkLog.activeCount > 0)
                     && bloodAlcohol.exposureTotal > 0
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: Display.exposureOf(bloodAlcohol.exposureTotal, sessionLog.roughExposure)
            font.pixelSize: Theme.fontSizeSmall
            color: Display.morningColor(bloodAlcohol.exposureTotal, advisor.nextExposure, sessionLog.roughExposure,
                                        Theme.secondaryColor)
        }
        Label {
            visible: profile.configured && drinkLog.count > 0 && advisor.verdict !== Advisor.Unknown
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: Display.verdictShort(advisor.verdict, advisor.nextDrinkAt, bloodAlcohol.now)
            color: Display.verdictColor(advisor.verdict)
        }
    }

    CoverActionList {
        enabled: profile.configured && drinkLog.count > 0 && drinkLog.activeCount === 0

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: drinkLog.repeatLatestDrink()
        }
    }

    CoverActionList {
        enabled: profile.configured && drinkLog.activeCount > 0

        CoverAction {
            iconSource: "image://theme/icon-cover-cancel"
            onTriggered: drinkLog.finishLatestDrink()
        }
        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: drinkLog.repeatLatestDrink()
        }
    }
}
