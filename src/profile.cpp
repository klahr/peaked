#include "profile.h"

#include <QStandardPaths>

static const char HeightKey[] = "profile/height";
static const char WeightKey[] = "profile/weight";
static const char AgeKey[] = "profile/age";
static const char SexKey[] = "profile/sex";
static const char LimitKey[] = "graph/limit";

Profile::Profile(QObject *parent)
    : QObject(parent)
    // The sandbox only allows writing inside the app's own config directory
    , m_settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                 + QStringLiteral("/settings.conf"), QSettings::IniFormat)
{
}

int Profile::height() const
{
    return m_settings.value(QLatin1String(HeightKey), 0).toInt();
}

int Profile::weight() const
{
    return m_settings.value(QLatin1String(WeightKey), 0).toInt();
}

int Profile::age() const
{
    return m_settings.value(QLatin1String(AgeKey), 0).toInt();
}

Profile::Sex Profile::sex() const
{
    return m_settings.value(QLatin1String(SexKey), Male).toInt() == Female ? Female : Male;
}

bool Profile::configured() const
{
    return height() > 0 && weight() > 0 && age() > 0;
}

double Profile::limit() const
{
    // The Swedish driving limit
    return m_settings.value(QLatin1String(LimitKey), 0.2).toDouble();
}

void Profile::save(int height, int weight, int age, Sex sex, double limit)
{
    m_settings.setValue(QLatin1String(HeightKey), height);
    m_settings.setValue(QLatin1String(WeightKey), weight);
    m_settings.setValue(QLatin1String(AgeKey), age);
    m_settings.setValue(QLatin1String(SexKey), static_cast<int>(sex));
    m_settings.setValue(QLatin1String(LimitKey), limit);
    m_settings.sync();
    emit changed();
}

double Profile::bodyWater() const
{
    if (!configured())
        return 0.0;
    const double water = sex() == Female
            ? -2.097 + 0.1069 * height() + 0.2466 * weight()
            : 2.447 - 0.09516 * age() + 0.1074 * height() + 0.3362 * weight();
    return qMax(water, 0.0);
}
