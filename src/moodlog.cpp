#include "moodlog.h"

#include <QStandardPaths>
#include <QVariantMap>

#include "bloodalcohol.h"

static const qint64 CooldownMs = 15 * 60 * 1000;

MoodLog::MoodLog(BloodAlcohol *bloodAlcohol, QObject *parent)
    : QObject(parent)
    , m_bloodAlcohol(bloodAlcohol)
    , m_settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                 + QStringLiteral("/moods.conf"), QSettings::IniFormat)
{
    m_cooldownTimer.setSingleShot(true);
    connect(&m_cooldownTimer, &QTimer::timeout, this, &MoodLog::canRecordChanged);
    load();
    startCooldown();
}

QVariantList MoodLog::entries() const
{
    QVariantList list;
    for (const Entry &entry : m_moods) {
        QVariantMap map;
        map[QStringLiteral("time")] = double(entry.time.toMSecsSinceEpoch());
        map[QStringLiteral("mood")] = int(entry.mood);
        map[QStringLiteral("perMille")] = entry.perMille;
        list.append(map);
    }
    return list;
}

QDateTime MoodLog::lastRecorded() const
{
    return m_moods.isEmpty() ? QDateTime() : m_moods.last().time;
}

bool MoodLog::canRecord() const
{
    return m_moods.isEmpty() || m_moods.last().time.msecsTo(QDateTime::currentDateTime()) >= CooldownMs;
}

void MoodLog::record(Mood mood)
{
    if (!canRecord())
        return;
    m_bloodAlcohol->update();
    m_moods.append({ QDateTime::currentDateTime(), mood, m_bloodAlcohol->current() });
    emit changed();
    emit canRecordChanged();
    startCooldown();
    save();
}

void MoodLog::recalculateBetween(const QDateTime &from, const QDateTime &to)
{
    bool touched = false;
    for (int i = m_moods.size() - 1; i >= 0; --i) {
        if (m_moods.at(i).time < from || m_moods.at(i).time > to)
            continue;
        touched = true;
        const double level = m_bloodAlcohol->levelAt(m_moods.at(i).time);
        if (level > 0.0)
            m_moods[i].perMille = level;
        else
            m_moods.removeAt(i);
    }
    if (!touched)
        return;
    m_cooldownTimer.stop();
    emit changed();
    emit canRecordChanged();
    startCooldown();
    save();
}

void MoodLog::startCooldown()
{
    if (canRecord())
        return;
    const qint64 remainingMs = CooldownMs - m_moods.last().time.msecsTo(QDateTime::currentDateTime());
    m_cooldownTimer.start(int(qMax<qint64>(0, remainingMs)));
}

void MoodLog::load()
{
    const int size = m_settings.beginReadArray(QStringLiteral("moods"));
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        Entry entry;
        entry.time = QDateTime::fromMSecsSinceEpoch(m_settings.value(QStringLiteral("time")).toLongLong());
        const int mood = m_settings.value(QStringLiteral("mood")).toInt();
        entry.mood = mood == Bad ? Bad : mood == Ok ? Ok : Good;
        entry.perMille = m_settings.value(QStringLiteral("perMille")).toDouble();
        m_moods.append(entry);
    }
    m_settings.endArray();
}

void MoodLog::save()
{
    m_settings.remove(QStringLiteral("moods"));
    m_settings.beginWriteArray(QStringLiteral("moods"), m_moods.size());
    for (int i = 0; i < m_moods.size(); ++i) {
        const Entry &entry = m_moods.at(i);
        m_settings.setArrayIndex(i);
        m_settings.setValue(QStringLiteral("time"), entry.time.toMSecsSinceEpoch());
        m_settings.setValue(QStringLiteral("mood"), int(entry.mood));
        m_settings.setValue(QStringLiteral("perMille"), entry.perMille);
    }
    m_settings.endArray();
    m_settings.sync();
}
