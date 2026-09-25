#include "drinklog.h"

#include <QRegExp>
#include <QStandardPaths>
#include <QUuid>

static const double EthanolDensity = 0.789; // grams per milliliter

static QString newId()
{
    return QUuid::createUuid().toString().remove(QRegExp(QStringLiteral("[{}-]")));
}

DrinkLog::DrinkLog(QObject *parent)
    : QAbstractListModel(parent)
    , m_settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                 + QStringLiteral("/drinks.conf"), QSettings::IniFormat)
{
    load();
    prune();
}

int DrinkLog::activeCount() const
{
    int active = 0;
    for (const Drink &drink : m_drinks) {
        if (!drink.finished.isValid())
            ++active;
    }
    return active;
}

QVariantList DrinkLog::entries() const
{
    QVariantList list;
    for (const Drink &drink : m_drinks) {
        QVariantMap map;
        map[QStringLiteral("started")] = double(drink.started.toMSecsSinceEpoch());
        map[QStringLiteral("abv")] = drink.abv;
        list.append(map);
    }
    return list;
}

int DrinkLog::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_drinks.size();
}

QVariant DrinkLog::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_drinks.size())
        return QVariant();
    const Drink &drink = m_drinks.at(index.row());
    switch (role) {
    case DrinkIdRole: return drink.id;
    case NameRole: return drink.name;
    case VolumeRole: return drink.volume;
    case AbvRole: return drink.abv;
    case GramsRole: return alcoholGrams(drink.volume, drink.abv);
    case StartedRole: return drink.started;
    case FinishedRole: return drink.finished;
    case ActiveRole: return !drink.finished.isValid();
    case ImageRole: return drink.image;
    default: return QVariant();
    }
}

QHash<int, QByteArray> DrinkLog::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[DrinkIdRole] = "drinkId";
    roles[NameRole] = "name";
    roles[VolumeRole] = "volume";
    roles[AbvRole] = "abv";
    roles[GramsRole] = "grams";
    roles[StartedRole] = "started";
    roles[FinishedRole] = "finished";
    roles[ActiveRole] = "active";
    roles[ImageRole] = "image";
    return roles;
}

void DrinkLog::startDrink(const QString &name, double volume, double abv, const QString &image)
{
    Drink drink;
    drink.id = newId();
    drink.name = name;
    drink.volume = volume;
    drink.abv = abv;
    drink.image = image;
    drink.started = QDateTime::currentDateTime();

    beginInsertRows(QModelIndex(), 0, 0);
    m_drinks.prepend(drink);
    endInsertRows();
    prune();
    emit countChanged();
    emit changed();
    save();
}

void DrinkLog::finishDrink(const QString &drinkId)
{
    const int row = indexOf(drinkId);
    if (row < 0 || m_drinks.at(row).finished.isValid())
        return;
    m_drinks[row].finished = QDateTime::currentDateTime();
    emit dataChanged(index(row), index(row));
    emit changed();
    save();
}

void DrinkLog::finishLatestDrink()
{
    for (const Drink &drink : m_drinks) {
        if (!drink.finished.isValid()) {
            finishDrink(drink.id);
            return;
        }
    }
}

void DrinkLog::repeatLatestDrink()
{
    if (m_drinks.isEmpty())
        return;
    const Drink latest = m_drinks.first();
    startDrink(latest.name, latest.volume, latest.abv, latest.image);
}

void DrinkLog::removeDrink(const QString &drinkId)
{
    const int row = indexOf(drinkId);
    if (row < 0)
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_drinks.removeAt(row);
    endRemoveRows();
    emit countChanged();
    emit changed();
    save();
}

void DrinkLog::updateDrink(const QString &drinkId, const QString &name, double volume, double abv,
                           const QDateTime &started, const QDateTime &finished)
{
    const int row = indexOf(drinkId);
    if (row < 0)
        return;
    Drink drink = m_drinks.at(row);
    drink.name = name.trimmed();
    drink.volume = volume;
    drink.abv = abv;
    drink.started = started;
    drink.finished = finished;
    m_drinks[row] = drink;
    emit dataChanged(index(row), index(row));

    // Keep newest first when the start time moved past another drink
    int target = 0;
    while (target < m_drinks.size() && (target == row || m_drinks.at(target).started > started))
        ++target;
    const int to = target > row ? target - 1 : target;
    if (to != row) {
        beginMoveRows(QModelIndex(), row, row, QModelIndex(), target);
        m_drinks.move(row, to);
        endMoveRows();
    }
    emit changed();
    save();
}

QVariantMap DrinkLog::drink(const QString &drinkId) const
{
    const int row = indexOf(drinkId);
    if (row < 0)
        return QVariantMap();
    const Drink &drink = m_drinks.at(row);
    QVariantMap map;
    map[QStringLiteral("name")] = drink.name;
    map[QStringLiteral("volume")] = drink.volume;
    map[QStringLiteral("abv")] = drink.abv;
    map[QStringLiteral("started")] = drink.started;
    map[QStringLiteral("finished")] = drink.finished;
    return map;
}

double DrinkLog::alcoholGrams(double volume, double abv)
{
    return volume * abv / 100.0 * EthanolDensity;
}

void DrinkLog::prune()
{
    const QDateTime cutoff = QDateTime::currentDateTime().addMSecs(-RelevantMs);
    bool pruned = false;
    for (int row = m_drinks.size() - 1; row >= 0; --row) {
        const QDateTime &finished = m_drinks.at(row).finished;
        if (!finished.isValid() || finished >= cutoff)
            continue;
        beginRemoveRows(QModelIndex(), row, row);
        m_drinks.removeAt(row);
        endRemoveRows();
        pruned = true;
    }
    if (pruned)
        save();
}

int DrinkLog::indexOf(const QString &drinkId) const
{
    for (int i = 0; i < m_drinks.size(); ++i) {
        if (m_drinks.at(i).id == drinkId)
            return i;
    }
    return -1;
}

void DrinkLog::load()
{
    const int size = m_settings.beginReadArray(QStringLiteral("drinks"));
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        Drink drink;
        drink.id = m_settings.value(QStringLiteral("id")).toString();
        drink.name = m_settings.value(QStringLiteral("name")).toString();
        drink.volume = m_settings.value(QStringLiteral("volume")).toDouble();
        drink.abv = m_settings.value(QStringLiteral("abv")).toDouble();
        drink.started = QDateTime::fromMSecsSinceEpoch(m_settings.value(QStringLiteral("started")).toLongLong());
        drink.image = m_settings.value(QStringLiteral("image")).toString();
        if (m_settings.contains(QStringLiteral("finished")))
            drink.finished = QDateTime::fromMSecsSinceEpoch(m_settings.value(QStringLiteral("finished")).toLongLong());
        m_drinks.append(drink);
    }
    m_settings.endArray();
}

void DrinkLog::save()
{
    m_settings.remove(QStringLiteral("drinks"));
    m_settings.beginWriteArray(QStringLiteral("drinks"), m_drinks.size());
    for (int i = 0; i < m_drinks.size(); ++i) {
        const Drink &drink = m_drinks.at(i);
        m_settings.setArrayIndex(i);
        m_settings.setValue(QStringLiteral("id"), drink.id);
        m_settings.setValue(QStringLiteral("name"), drink.name);
        m_settings.setValue(QStringLiteral("volume"), drink.volume);
        m_settings.setValue(QStringLiteral("abv"), drink.abv);
        m_settings.setValue(QStringLiteral("started"), drink.started.toMSecsSinceEpoch());
        if (drink.finished.isValid())
            m_settings.setValue(QStringLiteral("finished"), drink.finished.toMSecsSinceEpoch());
        if (!drink.image.isEmpty())
            m_settings.setValue(QStringLiteral("image"), drink.image);
    }
    m_settings.endArray();
    m_settings.sync();
}
