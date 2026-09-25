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
    return verdict === Advisor.Go ? moodColors[MoodLog.Good]
         : verdict === Advisor.Careful ? moodColors[MoodLog.Ok]
         : verdict === Advisor.Stop ? moodColors[MoodLog.Bad]
         : ""
}

function verdictShort(verdict) {
    return verdict === Advisor.Go ? qsTr("Go ahead")
         : verdict === Advisor.Careful ? qsTr("Take it easy")
         : verdict === Advisor.Stop ? qsTr("Better not")
         : ""
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
