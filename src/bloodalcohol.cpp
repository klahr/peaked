#include "bloodalcohol.h"

#include <QVector>
#include <QtMath>

#include "drinklog.h"
#include "profile.h"

// Widmark style model on Watson body water: alcohol is swallowed evenly over
// the time a drink is drunk, absorbed from the stomach with first order
// kinetics and burned at a constant rate.
static const double BloodWaterFraction = 0.8;
static const double AbsorptionRate = 4.0;   // per hour, about 10 minutes half time
static const double BurnRate = 0.15;        // per mille per hour
static const qint64 StepMs = 60 * 1000;
static const qint64 HourMs = 60 * 60 * 1000;
static const qint64 HorizonMs = 48 * HourMs;  // how far past the last drink to simulate at most
static const qint64 HistoryMs = 12 * HourMs;  // how far back the graph reaches
static const qint64 MarginMs = 15 * 60 * 1000;
static const qint64 EmptyHistoryMs = 2 * HourMs; // the graph without alcohol
static const qint64 EmptyAheadMs = HourMs;
static const double EmptyGrams = 0.01;
static const int MaxSamples = 240;

struct Intake {
    qint64 start;
    qint64 end;
    double grams;
};

static QVector<Intake> collectIntakes(const DrinkLog *log, qint64 nowMs)
{
    QVector<Intake> intakes;
    for (const DrinkLog::Drink &drink : log->drinks()) {
        const qint64 start = drink.started.toMSecsSinceEpoch();
        // An active drink counts as finished now, the safe assumption
        const qint64 end = drink.finished.isValid() ? drink.finished.toMSecsSinceEpoch()
                                                    : qMax(start, nowMs);
        if (end < nowMs - DrinkLog::RelevantMs)
            continue;
        intakes.append({ start, qMax(start, end), DrinkLog::alcoholGrams(drink.volume, drink.abv) });
    }
    return intakes;
}

static qint64 startOf(const QVector<Intake> &intakes)
{
    qint64 firstMs = intakes.first().start;
    for (const Intake &intake : intakes)
        firstMs = qMin(firstMs, intake.start);
    return firstMs - firstMs % StepMs;
}

// Returns the per mille at t0 + i * StepMs, until everything is burned
static QVector<double> simulate(const QVector<Intake> &intakes, qint64 t0, qint64 nowMs, double distribution)
{
    qint64 lastMs = 0;
    for (const Intake &intake : intakes)
        lastMs = qMax(lastMs, intake.end);
    const qint64 limitMs = qMax(nowMs, lastMs) + HorizonMs;
    const double absorbed = 1.0 - qExp(-AbsorptionRate * StepMs / HourMs);
    const double burned = BurnRate * distribution * StepMs / HourMs;
    QVector<double> values;
    values.append(0.0);
    double stomach = 0.0;
    double body = 0.0;
    for (qint64 t = t0; t < limitMs; t += StepMs) {
        for (const Intake &intake : intakes) {
            if (intake.end == intake.start) {
                if (intake.start >= t && intake.start < t + StepMs)
                    stomach += intake.grams;
                continue;
            }
            const qint64 overlap = qMin(intake.end, t + StepMs) - qMax(intake.start, t);
            if (overlap > 0)
                stomach += intake.grams * overlap / (intake.end - intake.start);
        }
        const double absorbedGrams = stomach * absorbed;
        stomach -= absorbedGrams;
        body = qMax(0.0, body + absorbedGrams - burned);
        values.append(body / distribution);
        if (t + StepMs > nowMs && t + StepMs > lastMs && body <= 0.0 && stomach < EmptyGrams)
            break;
    }
    return values;
}

// The stretches of values above zero. Each gets the drinks started since the
// previous stretch ended.
static QVector<BloodAlcohol::Episode> findEpisodes(const QVector<double> &values, qint64 t0,
                                                   const QVector<Intake> &intakes)
{
    QVector<BloodAlcohol::Episode> episodes;
    qint64 boundary = t0;
    int i = 0;
    while (i < values.size()) {
        if (values.at(i) <= 0.0) {
            ++i;
            continue;
        }
        BloodAlcohol::Episode episode { t0 + i * StepMs, 0, 0.0, 0.0, 0.0 };
        while (i < values.size() && values.at(i) > 0.0) {
            episode.peak = qMax(episode.peak, values.at(i));
            episode.exposure += values.at(i) * StepMs / HourMs;
            ++i;
        }
        episode.end = t0 + i * StepMs;
        for (const Intake &intake : intakes) {
            if (intake.start >= boundary && intake.start < episode.end)
                episode.grams += intake.grams;
        }
        boundary = episode.end;
        episodes.append(episode);
    }
    return episodes;
}

BloodAlcohol::BloodAlcohol(Profile *profile, DrinkLog *log, QObject *parent)
    : QObject(parent)
    , m_profile(profile)
    , m_log(log)
{
    connect(m_profile, &Profile::changed, this, &BloodAlcohol::update);
    connect(m_log, &DrinkLog::changed, this, &BloodAlcohol::update);
    connect(&m_timer, &QTimer::timeout, this, &BloodAlcohol::update);
    m_timer.start(30 * 1000);
    update();
}

void BloodAlcohol::update()
{
    m_now = QDateTime::currentDateTime();
    const qint64 nowMs = m_now.toMSecsSinceEpoch();
    m_current = 0.0;
    m_peak = 0.0;
    m_soberAt = QDateTime();
    m_graphStart = QDateTime();
    m_graphEnd = QDateTime();
    m_samples.clear();
    m_exposure = 0.0;
    m_exposureTotal = 0.0;
    m_episodes.clear();
    m_coveredFrom = -1;

    // Kilograms of blood equivalent, grams in the body divided by this is per mille
    const double distribution = m_profile->bodyWater() / BloodWaterFraction;
    const QVector<Intake> intakes = collectIntakes(m_log, nowMs);
    if (distribution <= 0.0 || intakes.isEmpty()) {
        showEmptyGraph(nowMs);
        emit changed();
        return;
    }

    const qint64 t0 = startOf(intakes);
    const QVector<double> values = simulate(intakes, t0, nowMs, distribution);
    m_coveredFrom = t0;
    m_episodes = findEpisodes(values, t0, intakes);
    for (const Episode &episode : m_episodes) {
        if (episode.start <= nowMs && episode.end > nowMs) {
            m_exposureTotal = episode.exposure;
            for (qint64 i = (episode.start - t0) / StepMs; i <= (nowMs - t0) / StepMs && i < values.size(); ++i)
                m_exposure += values.at(int(i)) * StepMs / HourMs;
        }
    }
    qint64 lastNonZeroMs = 0;
    for (int i = 0; i < values.size(); ++i) {
        if (values.at(i) > 0.0)
            lastNonZeroMs = t0 + i * StepMs;
    }

    auto valueAt = [&](qint64 ms) {
        const qint64 i = (ms - t0) / StepMs;
        return i >= 0 && i < values.size() ? values.at(int(i)) : 0.0;
    };

    m_current = valueAt(nowMs);
    if (lastNonZeroMs > nowMs)
        m_soberAt = QDateTime::fromMSecsSinceEpoch(lastNonZeroMs + StepMs);

    // Start the graph just before the first alcohol within the history window
    const qint64 historyMs = qMax(t0, nowMs - HistoryMs);
    qint64 startMs = -1;
    for (qint64 ms = historyMs - historyMs % StepMs; ms <= nowMs; ms += StepMs) {
        if (valueAt(ms) > 0.0) {
            startMs = qMax(historyMs, ms - MarginMs);
            break;
        }
    }
    if (startMs < 0 && m_current <= 0.0 && lastNonZeroMs <= nowMs) {
        showEmptyGraph(nowMs);
        emit changed();
        return;
    }
    if (startMs < 0)
        startMs = qMax(historyMs, nowMs - MarginMs);
    const qint64 endMs = qMax(nowMs, lastNonZeroMs) + MarginMs;

    const int count = int(qMin<qint64>(MaxSamples, (endMs - startMs) / StepMs + 1));
    for (int i = 0; i < count; ++i) {
        const double value = valueAt(startMs + (endMs - startMs) * i / qMax(1, count - 1));
        m_samples.append(value);
        m_peak = qMax(m_peak, value);
    }
    m_graphStart = QDateTime::fromMSecsSinceEpoch(startMs);
    m_graphEnd = QDateTime::fromMSecsSinceEpoch(endMs);
    m_hasAlcohol = true;
    emit changed();
}

double BloodAlcohol::peakWithDrink(double grams, qint64 durationMs, qint64 delayMs) const
{
    const double distribution = m_profile->bodyWater() / BloodWaterFraction;
    if (distribution <= 0.0)
        return 0.0;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    QVector<Intake> intakes = collectIntakes(m_log, nowMs);
    const qint64 startMs = nowMs + delayMs;
    intakes.append({ startMs, startMs + durationMs, grams });
    const qint64 t0 = startOf(intakes);
    const QVector<double> values = simulate(intakes, t0, startMs, distribution);
    double peak = 0.0;
    for (int i = int((startMs - t0) / StepMs); i < values.size(); ++i)
        peak = qMax(peak, values.at(i));
    return peak;
}

double BloodAlcohol::exposureWithDrink(double grams, qint64 durationMs) const
{
    const double distribution = m_profile->bodyWater() / BloodWaterFraction;
    if (distribution <= 0.0)
        return 0.0;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    QVector<Intake> intakes = collectIntakes(m_log, nowMs);
    intakes.append({ nowMs, nowMs + durationMs, grams });
    const qint64 t0 = startOf(intakes);
    const QVector<double> values = simulate(intakes, t0, nowMs, distribution);
    for (const Episode &episode : findEpisodes(values, t0, intakes)) {
        if (episode.end > nowMs)
            return episode.exposure;
    }
    return 0.0;
}

double BloodAlcohol::levelAt(const QDateTime &time) const
{
    const double distribution = m_profile->bodyWater() / BloodWaterFraction;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const QVector<Intake> intakes = collectIntakes(m_log, nowMs);
    if (distribution <= 0.0 || intakes.isEmpty())
        return 0.0;
    const qint64 t0 = startOf(intakes);
    const QVector<double> values = simulate(intakes, t0, nowMs, distribution);
    const qint64 i = (time.toMSecsSinceEpoch() - t0) / StepMs;
    return i >= 0 && i < values.size() ? values.at(int(i)) : 0.0;
}

QDateTime BloodAlcohol::soberAfter(const QDateTime &from) const
{
    const double distribution = m_profile->bodyWater() / BloodWaterFraction;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const QVector<Intake> intakes = collectIntakes(m_log, nowMs);
    if (distribution <= 0.0 || intakes.isEmpty())
        return from;
    const qint64 t0 = startOf(intakes);
    const QVector<double> values = simulate(intakes, t0, nowMs, distribution);
    bool risen = false;
    for (int i = qMax(0, int((from.toMSecsSinceEpoch() - t0) / StepMs)); i < values.size(); ++i) {
        if (values.at(i) > 0.0)
            risen = true;
        else if (risen)
            return QDateTime::fromMSecsSinceEpoch(t0 + i * StepMs);
    }
    return risen ? QDateTime::fromMSecsSinceEpoch(t0 + (values.size() - 1) * StepMs) : from;
}

// A flat line around now, so the graph is there before the first drink
void BloodAlcohol::showEmptyGraph(qint64 nowMs)
{
    m_hasAlcohol = false;
    m_samples = QVariantList() << 0.0 << 0.0;
    m_graphStart = QDateTime::fromMSecsSinceEpoch(nowMs - EmptyHistoryMs);
    m_graphEnd = QDateTime::fromMSecsSinceEpoch(nowMs + EmptyAheadMs);
}
