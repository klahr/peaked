import QtQuick 2.0
import Sailfish.Silica 1.0
import rs.r8.peaked 1.0
import "../display.js" as Display

// Picks a drink from Open Food Facts, whatever it knows is handed to picked()
Page {
    id: page

    signal picked(string name, string brand, real volume, real abv, string image)

    allowedOrientations: Orientation.All

    ProductSearch {
        id: productSearch
    }

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: productSearch.results

        header: Column {
            width: listView.width

            PageHeader {
                title: qsTr("Search drinks")
            }

            SearchField {
                id: searchField
                width: parent.width
                placeholderText: qsTr("Name or brand")
                EnterKey.iconSource: "image://theme/icon-m-search"
                EnterKey.onClicked: {
                    productSearch.search(text)
                    focus = false
                }
                Component.onCompleted: forceActiveFocus()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                text: qsTr("Data and photos from Open Food Facts, CC BY-SA. Missing size or alcohol is left for you to fill in.")
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusyIndicatorSize.Large
            running: productSearch.busy
        }

        ViewPlaceholder {
            enabled: !productSearch.busy && productSearch.errorString.length > 0
            text: productSearch.errorString
        }

        delegate: ListItem {
            id: listItem

            contentHeight: Theme.itemSizeMedium
            onClicked: {
                page.picked(modelData.name, modelData.brand, modelData.volume, modelData.abv,
                            modelData.image.length > 0 ? modelData.image : modelData.thumbnail)
                pageStack.pop()
            }

            DrinkPhoto {
                id: photo
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                source: modelData.thumbnail
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                x: photo.visible ? photo.x + photo.width + Theme.paddingMedium : Theme.horizontalPageMargin
                width: parent.width - x - Theme.horizontalPageMargin

                Label {
                    width: parent.width
                    text: modelData.name
                    truncationMode: TruncationMode.Fade
                    color: listItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    text: Display.details(modelData.brand,
                                          modelData.volume > 0 ? Display.volume(modelData.volume) : qsTr("size unknown"),
                                          modelData.abv > 0 ? Display.abv(modelData.abv) : qsTr("alcohol unknown"))
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: listItem.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
