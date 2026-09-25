import QtQuick 2.5
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0

// A drink's photo cropped to a rounded square, hidden without one
Item {
    property alias source: image.source

    visible: source.toString().length > 0
    width: Theme.itemSizeMedium - 2 * Theme.paddingSmall
    height: width

    Image {
        id: image
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        autoTransform: true
        asynchronous: true
        visible: false
    }

    Rectangle {
        id: mask
        anchors.fill: parent
        radius: Theme.paddingMedium
        visible: false
    }

    OpacityMask {
        anchors.fill: parent
        source: image
        maskSource: mask
    }
}
