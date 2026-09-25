#ifndef MOODLOG_H
#define MOODLOG_H

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QSettings>
#include <QTimer>
#include <QVariantList>

class BloodAlcohol;

// How the user felt, each with the estimated per mille at the time
class MoodLog : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "rs.r8.peaked")
    // Maps with time in milliseconds since the epoch, mood and perMille
    Q_PROPERTY(QVariantList entries READ entries NOTIFY changed)
    Q_PROPERTY(QDateTime lastRecorded READ lastRecorded NOTIFY changed)
    // False for a while after each recording
    Q_PROPERTY(bool canRecord READ canRecord NOTIFY canRecordChanged)

public:
    enum Mood {
        Good,
        Ok,
        Bad
    };
    Q_ENUM(Mood)

    struct Entry {
        QDateTime time;
        Mood mood;
        double perMille;
    };

    explicit MoodLog(BloodAlcohol *bloodAlcohol, QObject *parent = nullptr);

    const QList<Entry> &moods() const { return m_moods; }
    QVariantList entries() const;
    QDateTime lastRecorded() const;
    bool canRecord() const;

    Q_INVOKABLE void record(Mood mood);
    // After a drink was removed, for the moods from while it was in the blood.
    // Their per mille is taken again from the drinks left, a mood with no
    // alcohol left at its time is removed.
    Q_INVOKABLE void recalculateBetween(const QDateTime &from, const QDateTime &to);

public slots:
    // Called over D-Bus by the notification buttons, which can not pass arguments
    // of the right type
    Q_SCRIPTABLE void recordGood() { record(Good); }
    Q_SCRIPTABLE void recordOk() { record(Ok); }
    Q_SCRIPTABLE void recordBad() { record(Bad); }
    // Tapping the notification itself
    Q_SCRIPTABLE void activate() { emit activateRequested(); }

signals:
    void changed();
    void activateRequested();
    void canRecordChanged();

private:
    void load();
    void save();
    void startCooldown();

    BloodAlcohol *m_bloodAlcohol;
    QSettings m_settings;
    QTimer m_cooldownTimer;
    QList<Entry> m_moods; // oldest first
};

#endif // MOODLOG_H
