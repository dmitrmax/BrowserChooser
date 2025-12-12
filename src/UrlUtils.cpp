#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QByteArray>

QString unwrapSlackUrl(const QString &url, bool unwrapSlackUrls)
{
    if (!unwrapSlackUrls)
        return url;

    const QString prefix =
        "https://slack.com/openid/connect/login_initiate_redirect?login_hint=";

    if (!url.startsWith(prefix))
        return url;

    // Extract JWT from query
    QUrl qurl(url);
    QString jwt = qurl.query().mid(QString("login_hint=").length());

    // JWT format: header.payload.signature
    QStringList parts = jwt.split('.');
    if (parts.size() < 2)
        return url;

    // Decode payload (second part)
    QByteArray payload = QByteArray::fromBase64(parts[1].toUtf8(),
                                                QByteArray::Base64UrlEncoding |
                                                QByteArray::OmitTrailingEquals);

    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject())
        return url;

    QJsonObject obj = doc.object();
    QString targetUri = obj.value("https://slack.com/target_uri").toString();

    if (targetUri.isEmpty())
        return url;

    return targetUri;
}
