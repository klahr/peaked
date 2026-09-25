#ifndef PROFILE_H
#define PROFILE_H

#include <QObject>
#include <QSettings>

class Profile : public QObject
{
    Q_OBJECT
    // Centimeters, kilograms and years, zero until set
    Q_PROPERTY(int height READ height NOTIFY changed)
    Q_PROPERTY(int weight READ weight NOTIFY changed)
    Q_PROPERTY(int age READ age NOTIFY changed)
    Q_PROPERTY(Sex sex READ sex NOTIFY changed)
    Q_PROPERTY(bool configured READ configured NOTIFY changed)
    // Per mille where the graph draws its limit line
    Q_PROPERTY(double limit READ limit NOTIFY changed)

public:
    enum Sex {
        Male,
        Female
    };
    Q_ENUM(Sex)

    explicit Profile(QObject *parent = nullptr);

    int height() const;
    int weight() const;
    int age() const;
    Sex sex() const;
    bool configured() const;
    double limit() const;

    Q_INVOKABLE void save(int height, int weight, int age, Sex sex, double limit);

    // Total body water in liters from the Watson formula, zero until configured
    double bodyWater() const;

signals:
    void changed();

private:
    QSettings m_settings;
};

#endif // PROFILE_H
