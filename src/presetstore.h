#ifndef PRESETSTORE_H
#define PRESETSTORE_H

#include <QAbstractListModel>
#include <QList>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QVariantMap>

class PresetStore : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    // For crediting Open Food Facts where the photos are shown
    Q_PROPERTY(bool hasImages READ hasImages NOTIFY hasImagesChanged)
    // Where the camera page puts its shots until a preset is saved with one
    Q_PROPERTY(QString captureDirectory READ captureDirectory CONSTANT)

public:
    enum Roles {
        PresetIdRole = Qt::UserRole + 1,
        NameRole,
        BrandRole,
        VolumeRole,
        AbvRole,
        ImageRole
    };

    struct Preset {
        QString id;
        QString name;
        QString brand; // empty when not known
        double volume; // milliliters
        double abv;    // percent alcohol by volume
        QString image; // local file URL, empty without a photo
    };

    explicit PresetStore(QObject *parent = nullptr);

    int count() const { return m_presets.size(); }
    bool hasImages() const;
    QString captureDirectory() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Turns a camera shot upright as a small square in the capture directory,
    // the camera does not write the rotation into the file. viewfinderAspect is
    // the long over the short side of the viewfinder video, which may show less
    // than the photo. Returns its URL, empty when it could not be read.
    Q_INVOKABLE QString prepareCapture(const QString &path, int rotation, double viewfinderAspect) const;

    // Creates a preset when presetId is empty. Returns the id. An http(s)
    // image is downloaded and shown once it has arrived, empty removes the photo.
    Q_INVOKABLE QString savePreset(const QString &presetId, const QString &name, const QString &brand,
                                   double volume, double abv, const QString &image);
    Q_INVOKABLE void removePreset(const QString &presetId);
    Q_INVOKABLE QVariantMap preset(const QString &presetId) const;

signals:
    void countChanged();
    void hasImagesChanged();

private:
    int indexOf(const QString &presetId) const;
    void fetchImage(const QString &presetId, const QUrl &url);
    QString importImage(const QString &presetId, const QString &path) const;
    void setImage(int row, const QString &image);
    void load();
    void save();

    QSettings m_settings;
    QNetworkAccessManager m_network;
    QList<Preset> m_presets;
};

#endif // PRESETSTORE_H
