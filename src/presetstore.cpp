#include "presetstore.h"

#include <QRegExp>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QTransform>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUuid>

static QString imageDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/images");
}

// Only photos this store saved are ever deleted
static void removeImage(const QString &image)
{
    const QString path = QUrl(image).toLocalFile();
    if (!path.isEmpty() && path.startsWith(imageDirectory() + QLatin1Char('/')))
        QFile::remove(path);
}

static QString newId()
{
    return QUuid::createUuid().toString().remove(QRegExp(QStringLiteral("[{}-]")));
}

static const int PhotoSize = 400;

static QString captureDirectoryPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/captures");
}

// A new name each time, QML would show a cached photo for the same path
static QString newImagePath(const QString &presetId)
{
    QDir().mkpath(imageDirectory());
    return imageDirectory() + QLatin1Char('/') + presetId + QLatin1Char('-') + newId() + QStringLiteral(".jpg");
}

PresetStore::PresetStore(QObject *parent)
    : QAbstractListModel(parent)
    , m_settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                 + QStringLiteral("/presets.conf"), QSettings::IniFormat)
{
    // Shots left from dialogs that were cancelled
    QDir captures(captureDirectoryPath());
    captures.removeRecursively();
    captures.mkpath(QStringLiteral("."));
    load();
}

int PresetStore::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_presets.size();
}

QVariant PresetStore::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_presets.size())
        return QVariant();
    const Preset &preset = m_presets.at(index.row());
    switch (role) {
    case PresetIdRole: return preset.id;
    case NameRole: return preset.name;
    case BrandRole: return preset.brand;
    case VolumeRole: return preset.volume;
    case AbvRole: return preset.abv;
    case ImageRole: return preset.image;
    default: return QVariant();
    }
}

QHash<int, QByteArray> PresetStore::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PresetIdRole] = "presetId";
    roles[NameRole] = "name";
    roles[BrandRole] = "brand";
    roles[VolumeRole] = "volume";
    roles[AbvRole] = "abv";
    roles[ImageRole] = "image";
    return roles;
}

QString PresetStore::captureDirectory() const
{
    return captureDirectoryPath();
}

bool PresetStore::hasImages() const
{
    for (const Preset &preset : m_presets) {
        if (!preset.image.isEmpty())
            return true;
    }
    return false;
}

QString PresetStore::savePreset(const QString &presetId, const QString &name, const QString &brand,
                                double volume, double abv, const QString &image)
{
    const int row = indexOf(presetId);
    Preset preset;
    preset.id = presetId.isEmpty() ? newId() : presetId;
    preset.name = name.trimmed();
    preset.brand = brand.trimmed();
    preset.volume = volume;
    preset.abv = abv;

    // A new photo keeps the old one until it has been downloaded
    const QString oldImage = row < 0 ? QString() : m_presets.at(row).image;
    const QUrl imageUrl(image);
    const bool download = imageUrl.scheme() == QLatin1String("http") || imageUrl.scheme() == QLatin1String("https");
    const bool gallery = imageUrl.isLocalFile()
            && !imageUrl.toLocalFile().startsWith(imageDirectory() + QLatin1Char('/'));
    if (download) {
        preset.image = oldImage;
    } else if (gallery) {
        preset.image = importImage(preset.id, imageUrl.toLocalFile());
        if (preset.image.isEmpty())
            preset.image = oldImage;
        if (imageUrl.toLocalFile().startsWith(captureDirectoryPath() + QLatin1Char('/')))
            QFile::remove(imageUrl.toLocalFile());
    } else {
        preset.image = image;
    }
    if (oldImage != preset.image)
        removeImage(oldImage);

    if (row < 0) {
        beginInsertRows(QModelIndex(), m_presets.size(), m_presets.size());
        m_presets.append(preset);
        endInsertRows();
        emit countChanged();
    } else {
        m_presets[row] = preset;
        emit dataChanged(index(row), index(row));
    }
    save();
    emit hasImagesChanged();
    if (download)
        fetchImage(preset.id, imageUrl);
    return preset.id;
}

void PresetStore::fetchImage(const QString &presetId, const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Peaked/0.1.0 (klahr@r8.rs)"));
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, presetId]() {
        reply->deleteLater();
        const int row = indexOf(presetId);
        if (reply->error() != QNetworkReply::NoError || row < 0)
            return;
        const QString path = newImagePath(presetId);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(reply->readAll()) < 0)
            return;
        file.close();
        setImage(row, QUrl::fromLocalFile(path).toString());
    });
}

// A gallery photo is copied in as a small square, a camera photo would be far
// too large for the lists and would go if deleted from the gallery
// The centre square of a photo at PhotoSize, turned clockwise by rotation
// after any rotation the file itself asks for. keep is how much of the short
// side the square spans. Null when it can not be read.
static QImage squarePhoto(const QString &path, int rotation, double keep)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize size = reader.size();
    const int shortSide = int(PhotoSize / keep);
    if (size.isValid() && qMin(size.width(), size.height()) > shortSide)
        reader.setScaledSize(size.scaled(shortSide, shortSide, Qt::KeepAspectRatioByExpanding));
    QImage photo = reader.read();
    if (photo.isNull())
        return photo;

    const int side = int(qMin(photo.width(), photo.height()) * keep);
    photo = photo.copy((photo.width() - side) / 2, (photo.height() - side) / 2, side, side);
    if (side > PhotoSize)
        photo = photo.scaled(PhotoSize, PhotoSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (rotation % 360 != 0)
        photo = photo.transformed(QTransform().rotate(rotation));
    return photo;
}

QString PresetStore::importImage(const QString &presetId, const QString &path) const
{
    const QImage photo = squarePhoto(path, 0, 1.0);
    const QString target = newImagePath(presetId);
    if (photo.isNull() || !photo.save(target, "JPG", 90))
        return QString();
    return QUrl::fromLocalFile(target).toString();
}

QString PresetStore::prepareCapture(const QString &path, int rotation, double viewfinderAspect) const
{
    // A wider viewfinder shows the full long side but only part of the short
    // side, its square is that part of the photo's
    const QSize size = QImageReader(path).size();
    const double photoAspect = size.isValid() && qMin(size.width(), size.height()) > 0
            ? double(qMax(size.width(), size.height())) / qMin(size.width(), size.height()) : 1.0;
    const double keep = viewfinderAspect > photoAspect ? photoAspect / viewfinderAspect : 1.0;

    const QImage photo = squarePhoto(path, rotation, keep);
    QFile::remove(path);
    const QString target = captureDirectoryPath() + QLatin1Char('/') + newId() + QStringLiteral(".jpg");
    if (photo.isNull() || !photo.save(target, "JPG", 90))
        return QString();
    return QUrl::fromLocalFile(target).toString();
}

void PresetStore::setImage(int row, const QString &image)
{
    removeImage(m_presets.at(row).image);
    m_presets[row].image = image;
    emit dataChanged(index(row), index(row));
    emit hasImagesChanged();
    save();
}

void PresetStore::removePreset(const QString &presetId)
{
    const int row = indexOf(presetId);
    if (row < 0)
        return;
    removeImage(m_presets.at(row).image);
    beginRemoveRows(QModelIndex(), row, row);
    m_presets.removeAt(row);
    endRemoveRows();
    emit countChanged();
    emit hasImagesChanged();
    save();
}

QVariantMap PresetStore::preset(const QString &presetId) const
{
    const int row = indexOf(presetId);
    if (row < 0)
        return QVariantMap();
    const Preset &preset = m_presets.at(row);
    QVariantMap map;
    map[QStringLiteral("name")] = preset.name;
    map[QStringLiteral("brand")] = preset.brand;
    map[QStringLiteral("volume")] = preset.volume;
    map[QStringLiteral("abv")] = preset.abv;
    map[QStringLiteral("image")] = preset.image;
    return map;
}

int PresetStore::indexOf(const QString &presetId) const
{
    for (int i = 0; i < m_presets.size(); ++i) {
        if (m_presets.at(i).id == presetId)
            return i;
    }
    return -1;
}

void PresetStore::load()
{
    // A few common drinks to start from on first run
    if (!m_settings.value(QStringLiteral("seeded"), false).toBool()) {
        m_presets.append(Preset { newId(), tr("Beer"), QString(), 330.0, 5.0, QString() });
        m_presets.append(Preset { newId(), tr("Wine"), QString(), 150.0, 12.5, QString() });
        m_presets.append(Preset { newId(), tr("Shot"), QString(), 40.0, 40.0, QString() });
        m_settings.setValue(QStringLiteral("seeded"), true);
        save();
        return;
    }

    const int size = m_settings.beginReadArray(QStringLiteral("presets"));
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        Preset preset;
        preset.id = m_settings.value(QStringLiteral("id")).toString();
        preset.name = m_settings.value(QStringLiteral("name")).toString();
        preset.brand = m_settings.value(QStringLiteral("brand")).toString();
        preset.volume = m_settings.value(QStringLiteral("volume")).toDouble();
        preset.abv = m_settings.value(QStringLiteral("abv")).toDouble();
        preset.image = m_settings.value(QStringLiteral("image")).toString();
        m_presets.append(preset);
    }
    m_settings.endArray();
}

void PresetStore::save()
{
    m_settings.remove(QStringLiteral("presets"));
    m_settings.beginWriteArray(QStringLiteral("presets"), m_presets.size());
    for (int i = 0; i < m_presets.size(); ++i) {
        const Preset &preset = m_presets.at(i);
        m_settings.setArrayIndex(i);
        m_settings.setValue(QStringLiteral("id"), preset.id);
        m_settings.setValue(QStringLiteral("name"), preset.name);
        m_settings.setValue(QStringLiteral("brand"), preset.brand);
        m_settings.setValue(QStringLiteral("volume"), preset.volume);
        m_settings.setValue(QStringLiteral("abv"), preset.abv);
        m_settings.setValue(QStringLiteral("image"), preset.image);
    }
    m_settings.endArray();
    m_settings.sync();
}
