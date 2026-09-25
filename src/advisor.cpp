#include "advisor.h"

#include <QtMath>

#include "bloodalcohol.h"
#include "drinklog.h"
#include "moodlog.h"

static const double Window = 0.1;          // per mille either side of the peak
static const int MinimumMoods = 2;
static const qint64 DefaultDurationMs = 20 * 60 * 1000;
static const qint64 MaxDurationMs = 2 * 60 * 60 * 1000;
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
    m_drinkName.clear();
    m_nextPeak = 0.0;
    m_goodCount = 0;
    m_okCount = 0;
    m_badCount = 0;

    if (m_log->drinks().isEmpty()) {
        m_nextPeak = m_bloodAlcohol->peakWithDrink(StandardDrinkGrams, DefaultDurationMs);
    } else {
        // Assume it is drunk as fast as the latest one was
        const DrinkLog::Drink &latest = m_log->drinks().first();
        const qint64 durationMs = latest.finished.isValid()
                ? qBound<qint64>(60 * 1000, latest.started.msecsTo(latest.finished), MaxDurationMs)
                : DefaultDurationMs;
        m_drinkName = latest.name;
        m_nextPeak = m_bloodAlcohol->peakWithDrink(DrinkLog::alcoholGrams(latest.volume, latest.abv), durationMs);
    }

    for (const MoodLog::Entry &entry : m_moods->moods()) {
        if (qAbs(entry.perMille - m_nextPeak) > Window)
            continue;
        switch (entry.mood) {
        case MoodLog::Good: ++m_goodCount; break;
        case MoodLog::Ok: ++m_okCount; break;
        case MoodLog::Bad: ++m_badCount; break;
        }
    }

    const int total = m_goodCount + m_okCount + m_badCount;
    if (m_nextPeak > 0.0 && total >= MinimumMoods) {
        // Good counts for and bad against, ok is neutral
        const double score = double(m_goodCount - m_badCount) / total;
        m_verdict = score >= 1.0 / 3.0 ? Go : score <= -1.0 / 3.0 ? Stop : Careful;
    }
    emit changed();
}
