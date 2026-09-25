// Not a .pragma library, so Format and qsTr come from the importing component

function perMille(value) {
    return qsTr("%1 ‰").arg(value.toLocaleString(Qt.locale(), "f", 2))
}

function volume(milliliters) {
    return qsTr("%1 ml").arg(Math.round(milliliters))
}

function abv(percent) {
    return qsTr("%1 %").arg(percent.toLocaleString(Qt.locale(), "f", percent % 1 === 0 ? 0 : 1))
}

var moodEmojis = ["\uD83D\uDE00", "\uD83D\uDE10", "\uD83D\uDE41"]
var moodColors = ["#66bb6a", "#ffca28", "#ef5350"]

function moodEmoji(mood) {
    return moodEmojis[mood]
}

function moodColor(mood) {
    return moodColors[mood]
}

// Picked from the strength, the drinks have no type
function glass(abv) {
    return abv <= 8 ? "\uD83C\uDF7A" : abv <= 22 ? "\uD83C\uDF77" : "\uD83C\uDF78"
}

// Empty when there is too little to go on
function verdictColor(verdict) {
    return verdict === Advisor.Comfortable ? moodColors[MoodLog.Good]
         : verdict === Advisor.Careful || verdict === Advisor.Wait ? moodColors[MoodLog.Ok]
         : verdict === Advisor.Stop ? moodColors[MoodLog.Bad]
         : ""
}

// Informs about another drink, never recommends one
function verdictShort(verdict, nextDrinkAt, now) {
    return verdict === Advisor.Comfortable ? qsTr("Feels good")
         : verdict === Advisor.Careful ? qsTr("Take it easy")
         : verdict === Advisor.Wait ? qsTr("Wait %1").arg(duration(nextDrinkAt.getTime() - now.getTime()))
         : verdict === Advisor.Stop ? qsTr("Better stop")
         : ""
}

// Per mille hours, the area under the curve
function exposure(value) {
    return qsTr("%1 ‰·h").arg(value.toLocaleString(Qt.locale(), "f", 1))
}

// Tonight's exposure against the one mornings get rough from, like "1.6 / 2.5 ‰·h"
function exposureOf(value, rough) {
    return rough > 0 ? qsTr("%1 / %2").arg(value.toLocaleString(Qt.locale(), "f", 1)).arg(exposure(rough))
                     : exposure(value)
}

// Normal colour until the evening, or one more drink, reaches rough mornings
function morningColor(total, next, rough, normal) {
    return rough <= 0 ? normal
         : total >= rough ? moodColors[MoodLog.Bad]
         : next >= rough ? moodColors[MoodLog.Ok]
         : normal
}

// "Yesterday 19:10–02:30 · 5 standard drinks · 2.9 ‰·h"
function evening(start, end, grams, exposureValue, standardGrams) {
    var yesterday = new Date()
    yesterday.setDate(yesterday.getDate() - 1)
    var day = start.toDateString() === yesterday.toDateString() ? qsTr("Yesterday")
                                                               : Format.formatDate(start, Formatter.DateMedium)
    var drinks = Math.max(1, Math.round(grams / standardGrams))
    return qsTr("%1 %2–%3 · %4 · %5").arg(day).arg(time(start)).arg(time(end))
            .arg(drinks === 1 ? qsTr("1 standard drink") : qsTr("%1 standard drinks").arg(drinks))
            .arg(exposure(exposureValue))
}

// Rounded up to the minute, like "2 h 15 min"
function duration(milliseconds) {
    var minutes = Math.max(1, Math.ceil(milliseconds / 60000))
    var hours = Math.floor(minutes / 60)
    return hours > 0 ? qsTr("%1 h %2 min").arg(hours).arg(minutes % 60) : qsTr("%1 min").arg(minutes)
}

// "Brand · 330 ml · 5 %", without the brand when there is none
function details(brand, volumeText, abvText) {
    var text = qsTr("%1 · %2").arg(volumeText).arg(abvText)
    return brand.length > 0 ? qsTr("%1 · %2").arg(brand).arg(text) : text
}

function time(date) {
    return Format.formatDate(date, Formatter.TimeValue)
}
