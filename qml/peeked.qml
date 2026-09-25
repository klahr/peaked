import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.KeepAlive 1.2
import Nemo.Notifications 1.0
import "pages"
import "display.js" as Display

ApplicationWindow {
    id: window

    initialPage: Component { MainPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    function moodAction(name, displayName, method) {
        return {
            name: name,
            displayName: displayName,
            service: "rs.r8.peeked",
            path: "/rs/r8/peeked",
            iface: "rs.r8.peeked",
            method: method
        }
    }

    // Asks how the user feels every 30 minutes while there is alcohol in the blood.
    // Wakes the phone for it, a plain timer would not run while it sleeps.
    BackgroundJob {
        enabled: profile.configured && (bloodAlcohol.current > 0 || drinkLog.activeCount > 0)
        frequency: BackgroundJob.ThirtyMinutes
        onTriggered: {
            if (!moodLog.canRecord) {
                finished()
                return
            }
            bloodAlcohol.update()
            moodNotification.body = qsTr("At %1").arg(Display.perMille(bloodAlcohol.current))
            moodNotification.previewBody = moodNotification.body
            moodNotification.publish()
            finished()
        }
    }

    Notification {
        id: moodNotification

        appName: qsTr("Peeked")
        summary: qsTr("How do you feel?")
        previewSummary: summary
        remoteActions: [
            moodAction("default", "", "activate"),
            moodAction("good", qsTr("Good"), "recordGood"),
            moodAction("ok", qsTr("OK"), "recordOk"),
            moodAction("bad", qsTr("Bad"), "recordBad")
        ]
    }

    Connections {
        target: moodLog
        onChanged: moodNotification.close()
        onActivateRequested: window.activate()
    }

    // Timers do not run while the phone sleeps, catch up as soon as the app is shown
    Connections {
        target: Qt.application
        onStateChanged: {
            if (Qt.application.state === Qt.ApplicationActive)
                bloodAlcohol.update()
        }
    }

    // The buttons would do nothing once the app is gone
    Component.onDestruction: moodNotification.close()
}
