#ifndef SESSIONLOG_H
#define SESSIONLOG_H

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QSettings>

class BloodAlcohol;

// A summary of every evening, kept for good as the drinks are removed after
// two days, with how the user felt the morning after
class SessionLog : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "rs.r8.peaked.Morning")
    // The morning after an evening that has not been answered for yet
    Q_PROPERTY(bool morningPending READ morningPending NOTIFY changed)
    Q_PROPERTY(bool morningNotified READ morningNotified NOTIFY changed)
    // Per mille hours of the evening asked about
    Q_PROPERTY(double morningExposure READ morningExposure NOTIFY changed)
    // An unanswered evening that may still be asked about, for waking up
    Q_PROPERTY(bool morningUpcoming READ morningUpcoming NOTIFY changed)
    // Per mille hours from which mornings were mostly bad, zero when not known
    Q_PROPERTY(double roughExposure READ roughExposure NOTIFY changed)
    // Enough mornings answered to tell, rough or not
    Q_PROPERTY(bool morningsKnown READ morningsKnown NOTIFY changed)

public:
    struct Session {
        QDateTime start;
        QDateTime end;
        double grams;
        double peak;
        double exposure;
        int morning;   // MoodLog::Mood, -1 until answered
        bool notified; // the morning notification was sent
    };

    explicit SessionLog(BloodAlcohol *bloodAlcohol, QObject *parent = nullptr);

    bool morningPending() const { return m_pending >= 0; }
    bool morningNotified() const { return m_pending >= 0 && m_sessions.at(m_pending).notified; }
    double morningExposure() const { return m_pending >= 0 ? m_sessions.at(m_pending).exposure : 0.0; }
    bool morningUpcoming() const { return m_upcoming; }
    double roughExposure() const { return m_roughExposure; }
    bool morningsKnown() const { return m_morningsKnown; }

    // Mood as in MoodLog::Mood
    Q_INVOKABLE void recordMorning(int mood);
    Q_INVOKABLE void markNotified();

public slots:
    // Called over D-Bus by the notification buttons
    Q_SCRIPTABLE void recordGood() { recordMorning(0); }
    Q_SCRIPTABLE void recordOk() { recordMorning(1); }
    Q_SCRIPTABLE void recordBad() { recordMorning(2); }
    Q_SCRIPTABLE void activate() { emit activateRequested(); }

signals:
    void changed();
    void activateRequested();

private:
    void reconcile();
    void evaluate();
    void learn();
    void load();
    void save();

    BloodAlcohol *m_bloodAlcohol;
    QSettings m_settings;
    QList<Session> m_sessions; // oldest first
    int m_pending = -1;
    bool m_upcoming = false;
    double m_roughExposure = 0.0;
    bool m_morningsKnown = false;
};

#endif // SESSIONLOG_H
