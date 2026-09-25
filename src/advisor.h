#ifndef ADVISOR_H
#define ADVISOR_H

#include <QObject>

class BloodAlcohol;
class DrinkLog;
class MoodLog;

// Whether another of the latest drink is a good idea, from how the user felt
// before at the per mille it would bring them to
class Advisor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Verdict verdict READ verdict NOTIFY changed)
    // The latest drink, the one another of is considered. Empty when there are
    // no drinks, then a standard drink is considered instead.
    Q_PROPERTY(QString drinkName READ drinkName NOTIFY changed)
    Q_PROPERTY(double standardDrinkGrams READ standardDrinkGrams CONSTANT)
    Q_PROPERTY(double nextPeak READ nextPeak NOTIFY changed)
    // Moods recorded near nextPeak
    Q_PROPERTY(int goodCount READ goodCount NOTIFY changed)
    Q_PROPERTY(int okCount READ okCount NOTIFY changed)
    Q_PROPERTY(int badCount READ badCount NOTIFY changed)

public:
    enum Verdict {
        Unknown,
        Go,
        Careful,
        Stop
    };
    Q_ENUM(Verdict)

    Advisor(BloodAlcohol *bloodAlcohol, DrinkLog *log, MoodLog *moods, QObject *parent = nullptr);

    Verdict verdict() const { return m_verdict; }
    QString drinkName() const { return m_drinkName; }
    double standardDrinkGrams() const;
    double nextPeak() const { return m_nextPeak; }
    int goodCount() const { return m_goodCount; }
    int okCount() const { return m_okCount; }
    int badCount() const { return m_badCount; }

public slots:
    void update();

signals:
    void changed();

private:
    BloodAlcohol *m_bloodAlcohol;
    DrinkLog *m_log;
    MoodLog *m_moods;

    Verdict m_verdict = Unknown;
    QString m_drinkName;
    double m_nextPeak = 0.0;
    int m_goodCount = 0;
    int m_okCount = 0;
    int m_badCount = 0;
};

#endif // ADVISOR_H
