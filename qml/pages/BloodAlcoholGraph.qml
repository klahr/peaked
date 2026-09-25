import QtQuick 2.0
import Sailfish.Silica 1.0
import "../display.js" as Display

// Blood alcohol over time, what has happened drawn solid and the estimate
// ahead faded, with the limit dashed
Canvas {
    id: graph

    property var samples: []
    property date startTime
    property date endTime
    property date nowTime
    property real peak
    property real limit
    // How the user felt, drawn at the time and per mille it was recorded
    property var moods: []
    // Drawn as glasses along the bottom where each drink was started
    property var drinks: []

    readonly property real maxValue: Math.max(0.5, Math.ceil(Math.max(peak, limit) * 2 + 0.25) / 2)
    readonly property real labelWidth: Theme.fontSizeExtraSmall * 3
    readonly property real labelHeight: Theme.fontSizeExtraSmall * 1.5

    onSamplesChanged: requestPaint()
    onLimitChanged: requestPaint()
    onMoodsChanged: requestPaint()
    onDrinksChanged: requestPaint()
    // The drawn image is dropped while the app is in the background
    onAvailableChanged: if (available) requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        if (samples.length < 2)
            return

        var left = labelWidth
        var top = labelHeight / 2
        var plotWidth = width - left
        var plotHeight = height - top - labelHeight
        var start = startTime.getTime()
        var span = Math.max(1, endTime.getTime() - start)

        function xAt(ms) { return left + (ms - start) / span * plotWidth }
        function yAt(value) { return top + plotHeight * (1 - value / maxValue) }

        ctx.font = Theme.fontSizeExtraSmall + "px " + Theme.fontFamily
        ctx.lineWidth = 1

        // Per mille grid every half
        ctx.textAlign = "right"
        ctx.textBaseline = "middle"
        for (var value = 0; value <= maxValue + 0.001; value += 0.5) {
            ctx.strokeStyle = Theme.rgba(Theme.secondaryColor, 0.3)
            ctx.beginPath()
            ctx.moveTo(left, yAt(value))
            ctx.lineTo(width, yAt(value))
            ctx.stroke()
            ctx.fillStyle = Theme.secondaryColor
            ctx.fillText(value.toLocaleString(Qt.locale(), "f", 1), left - Theme.paddingSmall, yAt(value))
        }

        // Hour ticks, spaced out so the labels fit
        var hourMs = 60 * 60 * 1000
        var hourWidth = plotWidth * hourMs / span
        var hourStep = Math.max(1, Math.ceil(labelWidth * 1.2 / hourWidth))
        var firstHour = new Date(start)
        firstHour.setMinutes(0, 0, 0)
        ctx.textAlign = "center"
        ctx.textBaseline = "top"
        for (var hour = firstHour.getTime() + hourMs; hour < start + span; hour += hourMs) {
            if (new Date(hour).getHours() % hourStep !== 0)
                continue
            ctx.strokeStyle = Theme.rgba(Theme.secondaryColor, 0.3)
            ctx.beginPath()
            ctx.moveTo(xAt(hour), top)
            ctx.lineTo(xAt(hour), top + plotHeight)
            ctx.stroke()
            ctx.fillStyle = Theme.secondaryColor
            ctx.fillText(Qt.formatTime(new Date(hour), "HH"), xAt(hour), top + plotHeight + Theme.paddingSmall / 2)
        }

        // Canvas in Qt 5.6 has no setLineDash
        ctx.strokeStyle = Theme.errorColor
        ctx.beginPath()
        for (var dash = left; dash < width; dash += 2 * Theme.paddingSmall) {
            ctx.moveTo(dash, yAt(limit))
            ctx.lineTo(Math.min(width, dash + Theme.paddingSmall), yAt(limit))
        }
        ctx.stroke()

        var nowX = Math.min(width, Math.max(left, xAt(nowTime.getTime())))
        function curve() {
            ctx.beginPath()
            ctx.moveTo(left, yAt(0))
            for (var i = 0; i < samples.length; ++i)
                ctx.lineTo(left + i / (samples.length - 1) * plotWidth, yAt(samples[i]))
            ctx.lineTo(width, yAt(0))
            ctx.closePath()
        }

        // Past up to now
        ctx.save()
        ctx.beginPath()
        ctx.rect(left, 0, nowX - left, height)
        ctx.clip()
        curve()
        ctx.fillStyle = Theme.rgba(Theme.highlightBackgroundColor, 0.4)
        ctx.fill()
        ctx.lineWidth = Theme.paddingSmall / 2
        ctx.strokeStyle = Theme.highlightColor
        ctx.stroke()
        ctx.restore()

        // Estimate ahead
        ctx.save()
        ctx.beginPath()
        ctx.rect(nowX, 0, width - nowX, height)
        ctx.clip()
        curve()
        ctx.fillStyle = Theme.rgba(Theme.highlightBackgroundColor, 0.15)
        ctx.fill()
        ctx.lineWidth = Theme.paddingSmall / 2
        ctx.strokeStyle = Theme.rgba(Theme.highlightColor, 0.5)
        ctx.stroke()
        ctx.restore()

        ctx.strokeStyle = Theme.primaryColor
        ctx.lineWidth = 1
        ctx.beginPath()
        ctx.moveTo(nowX, top)
        ctx.lineTo(nowX, top + plotHeight)
        ctx.stroke()

        ctx.font = Theme.fontSizeMedium + "px " + Theme.fontFamily
        ctx.textAlign = "center"
        ctx.textBaseline = "bottom"
        ctx.fillStyle = Theme.secondaryHighlightColor
        for (var d = 0; d < drinks.length; ++d) {
            var drink = drinks[d]
            if (drink.started < start || drink.started > start + span)
                continue
            ctx.fillText(Display.glass(drink.abv), xAt(drink.started), yAt(0) - Theme.paddingSmall / 2)
        }

        ctx.textBaseline = "middle"
        for (var m = 0; m < moods.length; ++m) {
            var mood = moods[m]
            if (mood.time < start || mood.time > start + span)
                continue
            ctx.fillStyle = Display.moodColor(mood.mood)
            ctx.fillText(Display.moodEmoji(mood.mood), xAt(mood.time), yAt(Math.min(mood.perMille, maxValue)))
        }
    }
}
