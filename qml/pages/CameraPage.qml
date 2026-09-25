import QtQuick 2.0
import QtMultimedia 5.5
import Sailfish.Silica 1.0

// Takes a photo for a preset. The viewfinder shows the square that is kept.
Page {
    id: page

    signal captured(string url)

    allowedOrientations: Orientation.Portrait

    // Turns the viewfinder upright on a portrait page. Found by trying on an
    // Xperia 10 V, autoOrientation left it a quarter turn off.
    readonly property int viewfinderRotation: (270 - camera.orientation + 360) % 360
    // Clockwise turn that makes a shot upright, the camera writes no rotation
    // into the file. Found by trying on an Xperia 10 V, whose sensor reports 270.
    readonly property int captureRotation: (360 - camera.orientation) % 360
    property bool viewfinderChosen

    // A viewfinder with the shape of the photo, so its centre square is the
    // square that is kept
    function chooseViewfinder() {
        var photo = camera.imageCapture.resolution
        var resolutions = camera.supportedViewfinderResolutions()
        if (viewfinderChosen || photo.width <= 0 || resolutions.length === 0)
            return
        viewfinderChosen = true
        var best
        for (var i = 0; i < resolutions.length; ++i) {
            var r = resolutions[i]
            if (Math.abs(r.width / r.height - photo.width / photo.height) > 0.01 || r.width > 1920)
                continue
            if (!best || r.width > best.width)
                best = r
        }
        if (best)
            camera.viewfinder.resolution = Qt.size(best.width, best.height)
    }

    Camera {
        id: camera

        captureMode: Camera.CaptureStillImage
        focus.focusMode: Camera.FocusContinuous
        onCameraStatusChanged: {
            if (cameraStatus === Camera.ActiveStatus)
                page.chooseViewfinder()
        }

        imageCapture {
            onImageSaved: {
                var source = videoOutput.sourceRect
                var aspect = source.width > 0 && source.height > 0
                        ? Math.max(source.width, source.height) / Math.min(source.width, source.height) : 0
                var url = presetStore.prepareCapture(path, page.captureRotation, aspect)
                if (url.length === 0) {
                    errorLabel.text = qsTr("Could not read the photo")
                    return
                }
                page.captured(url)
                pageStack.pop()
            }
            onCaptureFailed: errorLabel.text = message
        }
    }

    // Fills the page, the camera's video is drawn misplaced when the output is
    // clipped or larger than its parent
    VideoOutput {
        id: videoOutput

        anchors.fill: parent
        source: camera
        orientation: page.viewfinderRotation
        fillMode: VideoOutput.PreserveAspectFit
    }

    // The centre square of the video, which is the part that is kept
    Item {
        id: viewfinder

        readonly property real side: Math.min(videoOutput.contentRect.width, videoOutput.contentRect.height)

        x: videoOutput.contentRect.x + (videoOutput.contentRect.width - side) / 2
        y: videoOutput.contentRect.y + (videoOutput.contentRect.height - side) / 2
        width: side
        height: side

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: Math.max(1, Theme.paddingSmall / 4)
            border.color: Theme.highlightColor
        }
    }

    // Dims the video outside the square
    Repeater {
        model: [
            Qt.rect(0, 0, page.width, viewfinder.y),
            Qt.rect(0, viewfinder.y + viewfinder.height, page.width, page.height - viewfinder.y - viewfinder.height),
            Qt.rect(0, viewfinder.y, viewfinder.x, viewfinder.height),
            Qt.rect(viewfinder.x + viewfinder.width, viewfinder.y,
                    page.width - viewfinder.x - viewfinder.width, viewfinder.height)
        ]

        Rectangle {
            x: modelData.x
            y: modelData.y
            width: modelData.width
            height: modelData.height
            color: Theme.rgba("black", 0.6)
        }
    }

    PageHeader {
        id: header
        title: qsTr("Take photo")
    }

    BusyIndicator {
        anchors.centerIn: viewfinder
        size: BusyIndicatorSize.Large
        running: camera.cameraStatus !== Camera.ActiveStatus && errorLabel.text.length === 0
    }

    Label {
        id: errorLabel

        anchors.centerIn: viewfinder
        width: viewfinder.width - 2 * Theme.horizontalPageMargin
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: camera.errorCode !== Camera.NoError ? camera.errorString : ""
        color: Theme.errorColor
    }

    IconButton {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: Theme.paddingLarge * 2
        }
        icon.source: "image://theme/icon-camera-shutter-release"
        enabled: camera.cameraStatus === Camera.ActiveStatus && camera.imageCapture.ready
        onClicked: camera.imageCapture.captureToLocation(presetStore.captureDirectory + "/" + Date.now() + ".jpg")
    }
}
