#ifndef ADVISOR_H
#define ADVISOR_H

#include <QDateTime>
#include <QObject>

class BloodAlcohol;
class DrinkLog;
class MoodLog;

// How the user is likely to feel after another of the latest drink, from how
// they felt before at the per mille it would bring them to and how they feel
// tonight. Informs, never recommends a drink.
class Advisor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Verdict verdict READ verdict NOTIFY changed)
    Q_PROPERTY(Reason reason READ reason NOTIFY changed)
    // The latest drink, the one another of is considered. Empty when there are
    // no drinks, then a standard drink is considered instead.
    Q_PROPERTY(QString drinkName READ drinkName NOTIFY changed)
    Q_PROPERTY(double standardDrinkGrams READ standardDrinkGrams CONSTANT)
    Q_PROPERTY(double nextPeak READ nextPeak NOTIFY changed)
    // Per mille hours of the evening with that drink added
    Q_PROPERTY(double nextExposure READ nextExposure NOTIFY changed)
    // Moods recorded near nextPeak
    Q_PROPERTY(int goodCount READ goodCount NOTIFY changed)
    Q_PROPERTY(int okCount READ okCount NOTIFY changed)
    Q_PROPERTY(int badCount READ badCount NOTIFY changed)
    // The highest per mille the user usually feels good at, zero when not known
    Q_PROPERTY(double comfortLevel READ comfortLevel NOTIFY changed)
    // For Wait, how long to wait before any next drink keeps its peak at waitPeak
    Q_PROPERTY(QDateTime nextDrinkAt READ nextDrinkAt NOTIFY changed)
    Q_PROPERTY(double waitPeak READ waitPeak NOTIFY changed)

public:
    enum Verdict {
        Unknown,
        Comfortable,
        Careful,
        Wait,
        Stop
    };
    Q_ENUM(Verdict)

    enum Reason {
        NoReason,
        History,          // how the user felt around nextPeak before
        BeyondExperience, // no moods recorded at that level
        TonightBad,       // the latest mood tonight was bad
        TonightWorse      // ok tonight after feeling good earlier
    };
    Q_ENUM(Reason)

    Advisor(BloodAlcohol *bloodAlcohol, DrinkLog *log, MoodLog *moods, QObject *parent = nullptr);

    Verdict verdict() const { return m_verdict; }
    Reason reason() const { return m_reason; }
    QString drinkName() const { return m_drinkName; }
    double standardDrinkGrams() const;
    double nextPeak() const { return m_nextPeak; }
    double nextExposure() const { return m_nextExposure; }
    int goodCount() const { return m_goodCount; }
    int okCount() const { return m_okCount; }
    int badCount() const { return m_badCount; }
    double comfortLevel() const { return m_comfortLevel; }
    QDateTime nextDrinkAt() const { return m_nextDrinkAt; }
    double waitPeak() const { return m_waitPeak; }

public slots:
    void update();

signals:
    void changed();

private:
    BloodAlcohol *m_bloodAlcohol;
    DrinkLog *m_log;
    MoodLog *m_moods;

    Verdict m_verdict = Unknown;
    Reason m_reason = NoReason;
    QString m_drinkName;
    double m_nextPeak = 0.0;
    double m_nextExposure = 0.0;
    int m_goodCount = 0;
    int m_okCount = 0;
    int m_badCount = 0;
    double m_comfortLevel = 0.0;
    QDateTime m_nextDrinkAt;
    double m_waitPeak = 0.0;
};

#endif // ADVISOR_H
