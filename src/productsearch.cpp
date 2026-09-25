#include "productsearch.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

static const int PageSize = 30;
// Larger than this is a bottle to pour from, not a serving
static const double MaxServing = 500.0;

// Milliliters in a quantity like "33 cl", "0.7l" or "300 cl (6 x 50 cl)",
// zero when there is none
static double parseVolume(const QString &quantity)
{
    static const QRegularExpression pack(QStringLiteral("x\\s*(\\d+(?:[.,]\\d+)?)\\s*(ml|cl|dl|l)\\b"),
                                         QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression single(QStringLiteral("(\\d+(?:[.,]\\d+)?)\\s*(ml|cl|dl|l)\\b"),
                                           QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = pack.match(quantity);
    if (!match.hasMatch())
        match = single.match(quantity);
    if (!match.hasMatch())
        return 0.0;
    const double value = match.captured(1).replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
    const QString unit = match.captured(2).toLower();
    const double factor = unit == QLatin1String("l") ? 1000.0
                        : unit == QLatin1String("dl") ? 100.0
                        : unit == QLatin1String("cl") ? 10.0 : 1.0;
    return value * factor;
}

// Percent in a name like "Norrlands Guld, 5,3%", zero when there is none
static double parseAbv(const QString &text)
{
    static const QRegularExpression percent(QStringLiteral("(\\d{1,2}(?:[.,]\\d+)?)\\s*%"));
    const QRegularExpressionMatch match = percent.match(text);
    return match.hasMatch() ? match.captured(1).replace(QLatin1Char(','), QLatin1Char('.')).toDouble() : 0.0;
}

ProductSearch::ProductSearch(QObject *parent)
    : QObject(parent)
{
}

void ProductSearch::search(const QString &query)
{
    if (m_reply)
        m_reply->abort();
    setError(QString());
    if (query.trimmed().isEmpty())
        return;

    QUrl url(QStringLiteral("https://search.openfoodfacts.org/search"));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("q"), query.trimmed());
    urlQuery.addQueryItem(QStringLiteral("page_size"), QString::number(PageSize));
    urlQuery.addQueryItem(QStringLiteral("fields"), QStringLiteral("product_name,brands,quantity,nutriments,"
                                                                   "image_front_thumb_url,image_front_small_url"));
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    // Open Food Facts asks every app to identify itself
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Peaked/0.1.0 (klahr@r8.rs)"));
    m_reply = m_network.get(request);
    connect(m_reply.data(), &QNetworkReply::finished, this, &ProductSearch::finished);
    emit busyChanged();
}

void ProductSearch::finished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply != m_reply)
        return;
    m_reply = nullptr;
    emit busyChanged();

    if (reply->error() == QNetworkReply::OperationCanceledError)
        return;
    if (reply->error() != QNetworkReply::NoError) {
        setError(tr("Search failed: %1").arg(reply->errorString()));
        return;
    }

    m_results.clear();
    const QJsonArray hits = QJsonDocument::fromJson(reply->readAll()).object().value(QStringLiteral("hits")).toArray();
    for (const QJsonValue &value : hits) {
        const QJsonObject hit = value.toObject();
        QString name = hit.value(QStringLiteral("product_name")).toString().trimmed();
        const QJsonArray brands = hit.value(QStringLiteral("brands")).toArray();
        const QString brand = brands.isEmpty() ? QString() : brands.first().toString().trimmed();
        if (name.isEmpty())
            name = brand;
        if (name.isEmpty())
            continue;

        const double volume = parseVolume(hit.value(QStringLiteral("quantity")).toString());
        const QJsonObject nutriments = hit.value(QStringLiteral("nutriments")).toObject();
        double abv = nutriments.value(QStringLiteral("alcohol_100g")).toDouble();
        if (abv <= 0.0)
            abv = parseAbv(name);

        QVariantMap result;
        result[QStringLiteral("name")] = name;
        result[QStringLiteral("brand")] = brand;
        result[QStringLiteral("volume")] = volume <= MaxServing ? volume : 0.0;
        result[QStringLiteral("abv")] = abv <= 100.0 ? abv : 0.0;
        result[QStringLiteral("thumbnail")] = hit.value(QStringLiteral("image_front_thumb_url")).toString();
        result[QStringLiteral("image")] = hit.value(QStringLiteral("image_front_small_url")).toString();
        m_results.append(result);
    }
    emit resultsChanged();
    if (m_results.isEmpty())
        setError(tr("No drinks found"));
}

void ProductSearch::setError(const QString &message)
{
    if (message == m_errorString)
        return;
    m_errorString = message;
    emit errorStringChanged();
}
