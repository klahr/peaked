#include "advisor.h"

#include <QMap>
#include <QVector>

#include "bloodalcohol.h"
#include "drinklog.h"
#include "monotone.h"
#include "moodlog.h"

static const double Window = 0.1;           // per mille either side of the peak, for the counts
static const double BinSize = 0.1;          // per mille
static const double MinimumWeight = 3.0;    // about three recent moods
static const double Threshold = 1.0 / 3.0;  // score for mostly good or mostly bad
static const qint64 DefaultDurationMs = 20 * 60 * 1000;
static const qint64 MaxDurationMs = 2 * 60 * 60 * 1000;
static const qint64 WaitStepMs = 5 * 60 * 1000;
static const qint64 MaxWaitMs = 90 * 60 * 1000;
static const double StandardDrinkGrams = 12.0; // a Swedish standardglas

Advisor::Advisor(BloodAlcohol *bloodAlcohol, DrinkLog *log, MoodLog *moods, QObject *parent)
    : QObject(parent)
    , m_bloodAlcohol(bloodAlcohol)
    , m_log(log)
    , m_moods(moods)
{
    // BloodAlcohol follows the drinks, the profile and the clock
    connect(m_bloodAlcohol, &BloodAlcohol::changed, this, &Advisor::update);
    connect(m_moods, &MoodLog::changed, this, &Advisor::update);
    update();
}

double Advisor::standardDrinkGrams() const
{
    return StandardDrinkGrams;
}

void Advisor::update()
{
    m_verdict = Unknown;
    m_reason = NoReason;
    m_drinkName.clear();
    m_nextPeak = 0.0;
    m_nextExposure = 0.0;
    m_goodCount = 0;
    m_okCount = 0;
    m_badCount = 0;
    m_comfortLevel = 0.0;
    m_nextDrinkAt = QDateTime();
    m_waitPeak = 0.0;

    // Assume it is drunk as fast as the latest one was
    double grams = StandardDrinkGrams;
    qint64 durationMs = DefaultDurationMs;
    if (!m_log->drinks().isEmpty()) {
        const DrinkLog::Drink &latest = m_log->drinks().first();
        if (latest.finished.isValid())
            durationMs = qBound<qint64>(60 * 1000, latest.started.msecsTo(latest.finished), MaxDurationMs);
        m_drinkName = latest.name;
        grams = DrinkLog::alcoholGrams(latest.volume, latest.abv);
    }
    m_nextPeak = m_bloodAlcohol->peakWithDrink(grams, durationMs);
    m_nextExposure = m_bloodAlcohol->exposureWithDrink(grams, durationMs);

    const QDateTime now = QDateTime::currentDateTime();
    QMap<int, ScoreBin> bins;
    double totalWeight = 0.0;
    double highest = 0.0;
    for (const MoodLog::Entry &entry : m_moods->moods()) {
        if (qAbs(entry.perMille - m_nextPeak) <= Window) {
            switch (entry.mood) {
            case MoodLog::Good: ++m_goodCount; break;
            case MoodLog::Ok: ++m_okCount; break;
            case MoodLog::Bad: ++m_badCount; break;
            }
        }
        const double weight = recencyWeight(entry.time, now);
        const double score = entry.mood == MoodLog::Good ? 1.0 : entry.mood == MoodLog::Bad ? -1.0 : 0.0;
        const int index = int(entry.perMille / BinSize);
        ScoreBin &bin = bins[index];
        bin.index = index;
        bin.score = (bin.score * bin.weight + score * weight) / (bin.weight + weight);
        bin.weight += weight;
        totalWeight += weight;
        highest = qMax(highest, entry.perMille);
    }

    if (m_nextPeak > 0.0 && totalWeight >= MinimumWeight) {
        const QVector<ScoreBin> fitted = monotoneDecreasing(bins.values().toVector());
        for (const ScoreBin &bin : fitted) {
            if (bin.score >= Threshold)
                m_comfortLevel = (bin.index + 1) * BinSize;
        }
        // The bin of the peak, or the closest one below it
        double score = fitted.first().score;
        for (const ScoreBin &bin : fitted) {
            if (bin.index * BinSize <= m_nextPeak)
                score = bin.score;
        }

        if (m_nextPeak > highest + BinSize) {
            m_verdict = score <= -Threshold ? Stop : Careful;
            m_reason = score <= -Threshold ? History : BeyondExperience;
        } else {
            m_verdict = score >= Threshold ? Comfortable : score <= -Threshold ? Stop : Careful;
            m_reason = History;
        }

        // Waiting lets the body burn some first, so any next peak is lower
        if (m_verdict != Comfortable && m_comfortLevel > 0.0) {
            for (qint64 delayMs = WaitStepMs; delayMs <= MaxWaitMs; delayMs += WaitStepMs) {
                const double peak = m_bloodAlcohol->peakWithDrink(grams, durationMs, delayMs);
                if (peak <= m_comfortLevel) {
                    m_verdict = Wait;
                    m_nextDrinkAt = now.addMSecs(delayMs);
                    m_waitPeak = peak;
                    break;
                }
            }
        }
    }

    // How it feels tonight weighs more than other evenings
    const bool drinking = m_bloodAlcohol->current() > 0.0 || m_log->activeCount() > 0;
    const QDateTime tonight = m_bloodAlcohol->graphStart();
    if (drinking && tonight.isValid()) {
        bool goodTonight = false;
        const MoodLog::Entry *latest = nullptr;
        for (const MoodLog::Entry &entry : m_moods->moods()) {
            if (entry.time < tonight)
                continue;
            if (latest && latest->mood == MoodLog::Good)
                goodTonight = true;
            latest = &entry;
        }
        if (latest && latest->mood == MoodLog::Bad) {
            m_verdict = Stop;
            m_reason = TonightBad;
        } else if (latest && latest->mood == MoodLog::Ok && goodTonight && m_verdict == Comfortable) {
            m_verdict = Careful;
            m_reason = TonightWorse;
        }
        if (m_reason == TonightBad || m_reason == TonightWorse) {
            m_nextDrinkAt = QDateTime();
            m_waitPeak = 0.0;
        }
    }
    emit changed();
}
