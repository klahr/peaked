#ifndef BLOODALCOHOL_H
#define BLOODALCOHOL_H

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QVariantList>

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

public:
    BloodAlcohol(Profile *profile, DrinkLog *log, QObject *parent = nullptr);

    double current() const { return m_current; }
    QDateTime soberAt() const { return m_soberAt; }
    QVariantList samples() const { return m_samples; }
    QDateTime graphStart() const { return m_graphStart; }
    QDateTime graphEnd() const { return m_graphEnd; }
    QDateTime now() const { return m_now; }
    double peak() const { return m_peak; }

    // Highest per mille ahead if another drink of the given grams of alcohol
    // was drunk from now over durationMs
    double peakWithDrink(double grams, qint64 durationMs) const;

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
    QDateTime m_soberAt;
    QDateTime m_graphStart;
    QDateTime m_graphEnd;
    QDateTime m_now;
    QVariantList m_samples;
};

#endif // BLOODALCOHOL_H
