#ifndef DRINKLOG_H
#define DRINKLOG_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QSettings>
#include <QVariantList>
#include <QVariantMap>

class DrinkLog : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY changed)
    // Maps with started in milliseconds since the epoch and abv, for the graph
    Q_PROPERTY(QVariantList entries READ entries NOTIFY changed)

public:
    enum Roles {
        DrinkIdRole = Qt::UserRole + 1,
        NameRole,
        VolumeRole,
        AbvRole,
        GramsRole,
        StartedRole,
        FinishedRole,
        ActiveRole,
        ImageRole
    };

    // The preset values are copied, so editing a preset leaves the log as it was
    struct Drink {
        QString id;
        QString name;
        double volume; // milliliters
        double abv;    // percent alcohol by volume
        QDateTime started;
        QDateTime finished; // invalid while the drink is active
        QString image;      // the preset's photo, empty without one
    };

    explicit DrinkLog(QObject *parent = nullptr);

    int count() const { return m_drinks.size(); }
    int activeCount() const;
    QVariantList entries() const;
    const QList<Drink> &drinks() const { return m_drinks; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void startDrink(const QString &name, double volume, double abv, const QString &image);
    Q_INVOKABLE void finishDrink(const QString &drinkId);
    // For the cover, which has no room to pick a drink
    Q_INVOKABLE void finishLatestDrink();
    Q_INVOKABLE void repeatLatestDrink();
    Q_INVOKABLE void removeDrink(const QString &drinkId);
    Q_INVOKABLE void updateDrink(const QString &drinkId, const QString &name, double volume, double abv,
                                 const QDateTime &started, const QDateTime &finished);
    Q_INVOKABLE QVariantMap drink(const QString &drinkId) const;

    static double alcoholGrams(double volume, double abv);

    // Drinks finished longer ago than this no longer affect the blood alcohol
    // and are removed
    static const qint64 RelevantMs = 48 * 60 * 60 * 1000;

signals:
    void countChanged();
    // Any change to the drinks, for recalculating the blood alcohol
    void changed();

private:
    int indexOf(const QString &drinkId) const;
    void load();
    void save();
    void prune();

    QSettings m_settings;
    QList<Drink> m_drinks; // newest first
};

#endif // DRINKLOG_H
