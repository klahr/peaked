#ifndef BLOODALCOHOL_H
#define BLOODALCOHOL_H

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVector>

class DrinkLog;
class Profile;

// Estimates blood alcohol in per mille from the drink log and the profile
class BloodAlcohol : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double current READ current NOTIFY changed)
    // Invalid when already sober
    Q_PROPERTY(QDateTime soberAt READ soberAt NOTIFY changed)
    // Evenly spaced per mille values from graphStart to graphEnd, a flat line
    // around now when there has been no alcohol in the blood for a long time
    Q_PROPERTY(QVariantList samples READ samples NOTIFY changed)
    Q_PROPERTY(QDateTime graphStart READ graphStart NOTIFY changed)
    Q_PROPERTY(QDateTime graphEnd READ graphEnd NOTIFY changed)
    Q_PROPERTY(QDateTime now READ now NOTIFY changed)
    Q_PROPERTY(double peak READ peak NOTIFY changed)
    // False while the graph is the flat line without alcohol
    Q_PROPERTY(bool hasAlcohol READ hasAlcohol NOTIFY changed)
    // Per mille hours, the area under the curve, of the evening going on: up
    // to now and all of it until sober. Zero when sober.
    Q_PROPERTY(double exposure READ exposure NOTIFY changed)
    Q_PROPERTY(double exposureTotal READ exposureTotal NOTIFY changed)

public:
    // A stretch with alcohol in the blood, from the drinks in the log
    struct Episode {
        qint64 start;
        qint64 end; // when sober again
        double grams;
        double peak;
        double exposure;
    };

    BloodAlcohol(Profile *profile, DrinkLog *log, QObject *parent = nullptr);

    double current() const { return m_current; }
    QDateTime soberAt() const { return m_soberAt; }
    QVariantList samples() const { return m_samples; }
    QDateTime graphStart() const { return m_graphStart; }
    QDateTime graphEnd() const { return m_graphEnd; }
    QDateTime now() const { return m_now; }
    double peak() const { return m_peak; }
    bool hasAlcohol() const { return m_hasAlcohol; }
    double exposure() const { return m_exposure; }
    double exposureTotal() const { return m_exposureTotal; }
    const QVector<Episode> &episodes() const { return m_episodes; }
    // Episodes are only known from here on, older drinks are gone. -1 without drinks.
    qint64 coveredFrom() const { return m_coveredFrom; }

    // Highest per mille from its start if another drink of the given grams of
    // alcohol was started delayMs from now and drunk over durationMs
    double peakWithDrink(double grams, qint64 durationMs, qint64 delayMs = 0) const;
    // Per mille hours of the evening if such a drink was started now
    double exposureWithDrink(double grams, qint64 durationMs) const;
    // When the blood alcohol first reaches zero after from, from when it never
    // rises. Up to then a drink started at from affects the level.
    Q_INVOKABLE QDateTime soberAfter(const QDateTime &from) const;
    // Per mille at time from the drinks now in the log
    double levelAt(const QDateTime &time) const;

public slots:
    void update();

signals:
    void changed();

private:
    void showEmptyGraph(qint64 nowMs);

    Profile *m_profile;
    DrinkLog *m_log;
    QTimer m_timer;

    double m_current = 0.0;
    double m_peak = 0.0;
    bool m_hasAlcohol = false;
    double m_exposure = 0.0;
    double m_exposureTotal = 0.0;
    QVector<Episode> m_episodes;
    qint64 m_coveredFrom = -1;
    QDateTime m_soberAt;
    QDateTime m_graphStart;
    QDateTime m_graphEnd;
    QDateTime m_now;
    QVariantList m_samples;
};

#endif // BLOODALCOHOL_H
