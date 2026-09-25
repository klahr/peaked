#ifndef PRODUCTSEARCH_H
#define PRODUCTSEARCH_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QVariantList>

class QNetworkReply;

// Looks drinks up by name in Open Food Facts to prefill presets
class ProductSearch : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    // Maps with name and brand, empty when not known, volume in milliliters and
    // abv, zero when not known, and
    // thumbnail and image URLs, empty without a photo
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    explicit ProductSearch(QObject *parent = nullptr);

    bool busy() const { return !m_reply.isNull(); }
    QVariantList results() const { return m_results; }
    QString errorString() const { return m_errorString; }

    Q_INVOKABLE void search(const QString &query);

signals:
    void busyChanged();
    void resultsChanged();
    void errorStringChanged();

private:
    void finished();
    void setError(const QString &message);

    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QVariantList m_results;
    QString m_errorString;
};

#endif // PRODUCTSEARCH_H
