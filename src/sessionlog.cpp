#include "sessionlog.h"

#include <QMap>
#include <QStandardPaths>
#include <QVector>

#include "bloodalcohol.h"
#include "monotone.h"

static const qint64 MinuteMs = 60 * 1000;
static const qint64 AskWithinMs = 24 * 60 * MinuteMs;  // after the evening ended
static const qint64 AskAfterMs = 30 * MinuteMs;
static const int MorningFromHour = 6;
static const int MorningUntilHour = 14;
static const double BinSize = 0.5;         // per mille hours
static const double MinimumWeight = 3.0;   // about three recent mornings
static const double Threshold = 1.0 / 3.0;

SessionLog::SessionLog(BloodAlcohol *bloodAlcohol, QObject *parent)
    : QObject(parent)
    , m_bloodAlcohol(bloodAlcohol)
    , m_settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                 + QStringLiteral("/sessions.conf"), QSettings::IniFormat)
{
    load();
    // BloodAlcohol follows the drinks and the clock
    connect(m_bloodAlcohol, &BloodAlcohol::changed, this, [this]() {
        reconcile();
        evaluate();
        emit changed();
    });
    reconcile();
    learn();
    evaluate();
}

void SessionLog::recordMorning(int mood)
{
    if (m_pending < 0)
        return;
    m_sessions[m_pending].morning = mood;
    save();
    learn();
    evaluate();
    emit changed();
}

void SessionLog::markNotified()
{
    if (m_pending < 0)
        return;
    m_sessions[m_pending].notified = true;
    save();
    emit changed();
}

// Evenings still covered by drinks follow them, so an edited or removed drink
// changes or removes its evening. Older ones are kept as they were.
void SessionLog::reconcile()
{
    const qint64 coveredFrom = m_bloodAlcohol->coveredFrom();
    if (coveredFrom < 0)
        return;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const QVector<BloodAlcohol::Episode> &episodes = m_bloodAlcohol->episodes();
    QVector<bool> used(episodes.size(), false);
    bool dirty = false;

    for (int s = m_sessions.size() - 1; s >= 0; --s) {
        Session &session = m_sessions[s];
        const qint64 start = session.start.toMSecsSinceEpoch();
        const qint64 end = session.end.toMSecsSinceEpoch();
        int match = -1;
        for (int e = 0; e < episodes.size(); ++e) {
            if (!used.at(e) && episodes.at(e).start < end && episodes.at(e).end > start)
                match = e;
        }
        if (start < coveredFrom) {
            // Its drinks are gone, what is left of it must not become a new evening
            if (match >= 0)
                used[match] = true;
            continue;
        }
        if (match < 0 || episodes.at(match).end > nowMs) {
            m_sessions.removeAt(s);
            dirty = true;
            continue;
        }
        const BloodAlcohol::Episode &episode = episodes.at(match);
        used[match] = true;
        if (session.start.toMSecsSinceEpoch() != episode.start || session.end.toMSecsSinceEpoch() != episode.end
                || session.grams != episode.grams) {
            session.start = QDateTime::fromMSecsSinceEpoch(episode.start);
            session.end = QDateTime::fromMSecsSinceEpoch(episode.end);
            session.grams = episode.grams;
            session.peak = episode.peak;
            session.exposure = episode.exposure;
            dirty = true;
        }
    }

    for (int e = 0; e < episodes.size(); ++e) {
        const BloodAlcohol::Episode &episode = episodes.at(e);
        if (used.at(e) || episode.end > nowMs || episode.grams <= 0.0)
            continue;
        Session session { QDateTime::fromMSecsSinceEpoch(episode.start), QDateTime::fromMSecsSinceEpoch(episode.end),
                          episode.grams, episode.peak, episode.exposure, -1, false };
        int at = 0;
        while (at < m_sessions.size() && m_sessions.at(at).start < session.start)
            ++at;
        m_sessions.insert(at, session);
        dirty = true;
    }

    if (dirty) {
        save();
        learn();
    }
}

// Asks about the latest unanswered evening the next morning
void SessionLog::evaluate()
{
    const QDateTime now = QDateTime::currentDateTime();
    const bool morning = now.time().hour() >= MorningFromHour && now.time().hour() < MorningUntilHour;
    const bool sober = m_bloodAlcohol->current() <= 0.0;
    m_pending = -1;
    m_upcoming = false;
    for (int s = m_sessions.size() - 1; s >= 0; --s) {
        const Session &session = m_sessions.at(s);
        const qint64 since = session.end.msecsTo(now);
        if (session.morning >= 0 || since > AskWithinMs)
            continue;
        m_upcoming = true;
        if (morning && sober && since >= AskAfterMs && session.start.date() < now.date()) {
            m_pending = s;
            break;
        }
    }
}

// The per mille hours from which mornings were mostly bad
void SessionLog::learn()
{
    const QDateTime now = QDateTime::currentDateTime();
    QMap<int, ScoreBin> bins;
    double totalWeight = 0.0;
    for (const Session &session : m_sessions) {
        if (session.morning < 0)
            continue;
        const double weight = recencyWeight(session.end, now);
        const double score = session.morning == 0 ? 1.0 : session.morning == 2 ? -1.0 : 0.0;
        const int index = int(session.exposure / BinSize);
        ScoreBin &bin = bins[index];
        bin.index = index;
        bin.score = (bin.score * bin.weight + score * weight) / (bin.weight + weight);
        bin.weight += weight;
        totalWeight += weight;
    }
    m_roughExposure = 0.0;
    m_morningsKnown = totalWeight >= MinimumWeight;
    if (!m_morningsKnown)
        return;
    for (const ScoreBin &bin : monotoneDecreasing(bins.values().toVector())) {
        if (bin.score <= -Threshold) {
            m_roughExposure = qMax(BinSize, bin.index * BinSize);
            break;
        }
    }
}

void SessionLog::load()
{
    const int size = m_settings.beginReadArray(QStringLiteral("sessions"));
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        Session session;
        session.start = QDateTime::fromMSecsSinceEpoch(m_settings.value(QStringLiteral("start")).toLongLong());
        session.end = QDateTime::fromMSecsSinceEpoch(m_settings.value(QStringLiteral("end")).toLongLong());
        session.grams = m_settings.value(QStringLiteral("grams")).toDouble();
        session.peak = m_settings.value(QStringLiteral("peak")).toDouble();
        session.exposure = m_settings.value(QStringLiteral("exposure")).toDouble();
        session.morning = m_settings.value(QStringLiteral("morning"), -1).toInt();
        session.notified = m_settings.value(QStringLiteral("notified"), false).toBool();
        m_sessions.append(session);
    }
    m_settings.endArray();
}

void SessionLog::save()
{
    m_settings.remove(QStringLiteral("sessions"));
    m_settings.beginWriteArray(QStringLiteral("sessions"), m_sessions.size());
    for (int i = 0; i < m_sessions.size(); ++i) {
        const Session &session = m_sessions.at(i);
        m_settings.setArrayIndex(i);
        m_settings.setValue(QStringLiteral("start"), session.start.toMSecsSinceEpoch());
        m_settings.setValue(QStringLiteral("end"), session.end.toMSecsSinceEpoch());
        m_settings.setValue(QStringLiteral("grams"), session.grams);
        m_settings.setValue(QStringLiteral("peak"), session.peak);
        m_settings.setValue(QStringLiteral("exposure"), session.exposure);
        m_settings.setValue(QStringLiteral("morning"), session.morning);
        m_settings.setValue(QStringLiteral("notified"), session.notified);
    }
    m_settings.endArray();
    m_settings.sync();
}
