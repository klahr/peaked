import QtQuick 2.0
import Sailfish.Silica 1.0
import "../display.js" as Display
import rs.r8.peaked 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    // Moods are only asked for while there is alcohol in the blood
    readonly property bool drinking: profile.configured
                                     && (bloodAlcohol.current > 0 || drinkLog.activeCount > 0)

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: drinkLog

        // The header grows as its content loads, which would leave the top of
        // it scrolled away until the user has scrolled themselves
        property bool userScrolled
        onMovementStarted: userScrolled = true

        // Not while the header is still being created
        Timer {
            id: toTop
            interval: 0
            onTriggered: listView.positionViewAtBeginning()
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsDialog.qml"))
            }
            MenuItem {
                text: qsTr("Presets")
                onClicked: pageStack.push(Qt.resolvedUrl("PresetsPage.qml"))
            }
            MenuItem {
                text: qsTr("Start drink")
                enabled: profile.configured
                onClicked: pageStack.push(Qt.resolvedUrl("PresetsPage.qml"), { picking: true })
            }
        }

        header: Column {
            width: listView.width
            onHeightChanged: {
                if (!listView.userScrolled)
                    toTop.restart()
            }
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Peaked")
            }

            Column {
                visible: !profile.configured
                width: parent.width
                spacing: Theme.paddingLarge

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Enter your height, weight, age and sex to estimate your blood alcohol")
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeLarge
                }
                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Settings")
                    onClicked: pageStack.push(Qt.resolvedUrl("SettingsDialog.qml"))
                }
            }

            Label {
                visible: profile.configured
                anchors.horizontalCenter: parent.horizontalCenter
                text: Display.perMille(bloodAlcohol.current)
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeHuge
            }

            Label {
                visible: profile.configured && !isNaN(bloodAlcohol.soberAt.getTime())
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Sober at %1").arg(Display.time(bloodAlcohol.soberAt))
                color: Theme.secondaryHighlightColor
            }

            Label {
                visible: page.drinking && bloodAlcohol.exposureTotal > 0
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Exposure %1 so far, %2 until sober").arg(Display.exposure(bloodAlcohol.exposure))
                                                               .arg(Display.exposure(bloodAlcohol.exposureTotal))
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }

            Column {
                visible: profile.configured && sessionLog.morningPending
                width: parent.width
                spacing: Theme.paddingMedium

                SectionHeader {
                    text: qsTr("How do you feel after yesterday?")
                }
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    wrapMode: Text.Wrap
                    text: Display.evening(sessionLog.morningStart, sessionLog.morningEnd, sessionLog.morningGrams,
                                          sessionLog.morningExposure, advisor.standardDrinkGrams)
                    color: Theme.secondaryHighlightColor
                    font.pixelSize: Theme.fontSizeSmall
                }
                MoodButtons {
                    onPicked: sessionLog.recordMorning(mood)
                }
            }

            BloodAlcoholGraph {
                visible: profile.configured && bloodAlcohol.samples.length > 0
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                height: Math.round(page.height / 3)
                samples: bloodAlcohol.samples
                startTime: bloodAlcohol.graphStart
                endTime: bloodAlcohol.graphEnd
                nowTime: bloodAlcohol.now
                peak: bloodAlcohol.peak
                limit: profile.limit
                // Without alcohol there is nothing the moods belong to
                moods: bloodAlcohol.hasAlcohol ? moodLog.entries : []
                drinks: drinkLog.entries
            }

            Column {
                visible: profile.configured
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                spacing: Theme.paddingSmall

                Label {
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: advisor.drinkName.length > 0
                          ? qsTr("Another %1 would take you to %2").arg(advisor.drinkName)
                                                                   .arg(Display.perMille(advisor.nextPeak))
                          : qsTr("A standard drink (%1 g) would take you to %2").arg(advisor.standardDrinkGrams)
                                                                                .arg(Display.perMille(advisor.nextPeak))
                    color: Theme.highlightColor
                }
                // Moods only come in once there are drinks to relate them to
                Label {
                    visible: drinkLog.count > 0
                             && (advisor.verdict !== Advisor.Unknown
                                 || advisor.goodCount + advisor.okCount + advisor.badCount > 0)
                    width: parent.width
                    wrapMode: Text.Wrap
                    // Informs about another drink, never recommends one
                    text: advisor.verdict === Advisor.Comfortable ? qsTr("You usually feel good around that level")
                        : advisor.verdict === Advisor.Careful
                          ? (advisor.reason === Advisor.BeyondExperience ? qsTr("Take it easy, you have not recorded a mood at that level yet")
                             : advisor.reason === Advisor.TonightWorse ? qsTr("Take it easy, you feel worse than earlier tonight")
                             : qsTr("Take it easy, you have felt mixed around that level"))
                        : advisor.verdict === Advisor.Wait
                          ? qsTr("Have a glass of water. If you have another %1, wait at least %2 to stay at %3")
                            .arg(advisor.drinkName.length > 0 ? advisor.drinkName : qsTr("drink"))
                            .arg(Display.duration(advisor.nextDrinkAt.getTime() - bloodAlcohol.now.getTime()))
                            .arg(Display.perMille(advisor.waitPeak))
                        : advisor.verdict === Advisor.Stop
                          ? (advisor.reason === Advisor.TonightBad ? qsTr("Better stop for tonight, you feel bad")
                             : qsTr("Better stop, you have felt bad around that level"))
                        : advisor.goodCount + advisor.okCount + advisor.badCount === 0
                          ? qsTr("No moods recorded around that level yet")
                          : qsTr("Too few moods recorded around that level")
                    color: advisor.verdict === Advisor.Unknown ? Theme.secondaryHighlightColor
                                                               : Display.verdictColor(advisor.verdict)
                    font.pixelSize: Theme.fontSizeLarge
                }
                Label {
                    visible: drinkLog.count > 0 && advisor.goodCount + advisor.okCount + advisor.badCount > 0
                    width: parent.width
                    text: qsTr("%1 %2   %3 %4   %5 %6")
                          .arg(Display.moodEmoji(MoodLog.Good)).arg(advisor.goodCount)
                          .arg(Display.moodEmoji(MoodLog.Ok)).arg(advisor.okCount)
                          .arg(Display.moodEmoji(MoodLog.Bad)).arg(advisor.badCount)
                    color: Theme.secondaryHighlightColor
                    font.pixelSize: Theme.fontSizeSmall
                }
                // About tomorrow, from how mornings after evenings like this felt.
                // Only once enough mornings are answered to say something.
                Label {
                    readonly property double rough: sessionLog.roughExposure
                    readonly property double total: bloodAlcohol.exposureTotal
                    visible: drinkLog.count > 0 && page.drinking && total > 0 && sessionLog.morningsKnown
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: rough > 0 && total >= rough
                          ? qsTr("Mornings after evenings above %1 were usually rough").arg(Display.exposure(rough))
                          : rough > 0 && advisor.nextExposure >= rough
                          ? qsTr("Another %1 would take the evening to %2. Mornings after that were usually rough")
                            .arg(advisor.drinkName.length > 0 ? advisor.drinkName : qsTr("drink"))
                            .arg(Display.exposure(advisor.nextExposure))
                          : rough > 0
                          ? qsTr("Rough mornings from about %1, this evening %2").arg(Display.exposure(rough))
                                                                                .arg(Display.exposure(total))
                          : qsTr("No rough mornings recorded yet, this evening %1").arg(Display.exposure(total))
                    color: Display.morningColor(total, advisor.nextExposure, rough, Theme.secondaryHighlightColor)
                }
                Button {
                    visible: advisor.drinkName.length > 0
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Start another %1").arg(advisor.drinkName)
                    onClicked: drinkLog.repeatLatestDrink()
                }
            }

            SectionHeader {
                visible: moodRow.visible
                text: qsTr("How do you feel?")
            }

            MoodButtons {
                id: moodRow
                visible: page.drinking && moodLog.canRecord
                onPicked: moodLog.record(mood)
            }

            Label {
                visible: page.drinking && !moodLog.canRecord
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Mood recorded at %1").arg(Display.time(moodLog.lastRecorded))
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }

            Label {
                visible: profile.configured && drinkLog.count === 0
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Pull down to start a drink")
                color: Theme.secondaryHighlightColor
            }

            SectionHeader {
                visible: drinkLog.count > 0
                text: qsTr("Drinks")
            }
        }

        delegate: ListItem {
            id: listItem

            contentHeight: Theme.itemSizeMedium
            menu: ContextMenu {
                MenuItem {
                    visible: model.active
                    text: qsTr("Finish")
                    onClicked: drinkLog.finishDrink(model.drinkId)
                }
                MenuItem {
                    visible: !model.active
                    text: qsTr("Edit")
                    onClicked: pageStack.push(Qt.resolvedUrl("DrinkDialog.qml"), { drinkId: model.drinkId })
                }
                MenuItem {
                    text: qsTr("Remove")
                    onClicked: {
                        var drinkId = model.drinkId
                        var started = model.started
                        listItem.remorseDelete(function() {
                            // Moods from while it was in the blood get the per mille
                            // of the drinks left, or go when nothing else was drunk
                            var sober = bloodAlcohol.soberAfter(started)
                            drinkLog.removeDrink(drinkId)
                            moodLog.recalculateBetween(started, sober)
                        })
                    }
                }
            }
            onClicked: {
                if (model.active)
                    drinkLog.finishDrink(model.drinkId)
                else
                    pageStack.push(Qt.resolvedUrl("DrinkDialog.qml"), { drinkId: model.drinkId })
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
                    color: model.active || listItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    text: model.active
                          ? qsTr("Drinking since %1, tap to finish").arg(Display.time(model.started))
                          : qsTr("%1 – %2 · %3 · %4").arg(Display.time(model.started)).arg(Display.time(model.finished))
                            .arg(Display.volume(model.volume)).arg(Display.abv(model.abv))
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: model.active || listItem.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
