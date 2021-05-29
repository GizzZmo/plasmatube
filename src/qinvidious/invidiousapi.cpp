// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "invidiousapi.h"
// Qt
#include <QFutureInterface>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringBuilder>
#include <QUrl>
#include <QUrlQuery>

constexpr QStringView API_FEED = u"/api/v1/auth/feed";
constexpr QStringView API_LOGIN = u"/login";
constexpr QStringView API_SEARCH = u"/api/v1/search";
constexpr QStringView API_SUBSCRIPTIONS = u"/api/v1/auth/subscriptions";
constexpr QStringView API_TOP = u"/api/v1/top";
constexpr QStringView API_TRENDING = u"/api/v1/trending";
constexpr QStringView API_VIDEOS = u"/api/v1/videos";

using namespace QInvidious;

InvidiousApi::InvidiousApi(QNetworkAccessManager *netManager, QObject *parent)
    : QObject(parent),
      m_netManager(netManager)
{
}

QString InvidiousApi::region() const
{
    return m_region;
}

void InvidiousApi::setRegion(const QString &region)
{
    m_region = region;
}

QString InvidiousApi::language() const
{
    return m_language;
}

void InvidiousApi::setLanguage(const QString &language)
{
    m_language = language;
}

Credentials InvidiousApi::credentials() const
{
    return m_credentials;
}

void InvidiousApi::setCredentials(const Credentials &credentials)
{
    m_credentials = credentials;
    emit credentialsChanged();
}

void InvidiousApi::setCredentials(const QString &apiInstance)
{
    m_credentials = Credentials();
    m_credentials.setApiInstance(apiInstance);
    emit credentialsChanged();
}

QString InvidiousApi::invidiousInstance() const
{
    return m_credentials.apiInstance();
}

QFuture<LogInResult> InvidiousApi::logIn(QStringView username, QStringView password)
{
    QUrlQuery params;
    params.addQueryItem("email", QUrl::toPercentEncoding(username.toString()));
    params.addQueryItem("password", QUrl::toPercentEncoding(password.toString()));
    params.addQueryItem("action", "signin");

    QNetworkRequest request(logInUrl());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    return post<LogInResult>(std::move(request), params.toString().toUtf8(),
                             [=](QNetworkReply *reply) -> LogInResult {
        const auto cookies = reply->header(QNetworkRequest::SetCookieHeader).value<QList<QNetworkCookie>>();

        if (!cookies.isEmpty()) {
            m_credentials.setUsername(username);
            m_credentials.setCookie(cookies.first());
            emit credentialsChanged();
            return m_credentials;
        }
        return std::pair(QNetworkReply::ContentAccessDenied, tr("Username or password is wrong."));
    });
}

QFuture<VideoResult> InvidiousApi::requestVideo(QStringView videoId)
{
    return get<VideoResult>(QNetworkRequest(videoUrl(videoId)),
                            [=](QNetworkReply *reply) -> VideoResult {
        if (auto doc = QJsonDocument::fromJson(reply->readAll()); !doc.isNull()) {
            return Video::fromJson(doc);
        }
        return invalidJsonError();
    });
}

QFuture<VideoListResult> InvidiousApi::requestSearchResults(QStringView query, qint32 page)
{
    return requestVideoList(Search, query, page);
}

QFuture<VideoListResult> InvidiousApi::requestFeed()
{
    return requestVideoList(Feed);
}

QFuture<VideoListResult> InvidiousApi::requestTop()
{
    return requestVideoList(Top);
}

QFuture<VideoListResult> InvidiousApi::requestTrending(TrendingTopic topic)
{
    switch (topic) {
    case Music:
        return requestVideoList(Trending, u"music");
    case Gaming:
        return requestVideoList(Trending, u"gaming");
    case Movies:
        return requestVideoList(Trending, u"movies");
    case News:
        return requestVideoList(Trending, u"news");
    default:
        return requestVideoList(Trending);
    }
}

QFuture<SubscriptionsResult> InvidiousApi::requestSubscriptions()
{
    return get<SubscriptionsResult>(authenticatedNetworkRequest(subscriptionsUrl()),
                                    [=](QNetworkReply *reply) -> SubscriptionsResult {
        if (auto doc = QJsonDocument::fromJson(reply->readAll()); !doc.isNull()) {
            auto array = doc.array();

            QList<QString> subscriptions;
            std::transform(array.cbegin(), array.cend(), std::back_inserter(subscriptions),
                           [](const QJsonValue &val) {
                return val.toObject().value(QStringLiteral("authorId")).toString();
            });
            return subscriptions;
        }
        return invalidJsonError();
    });
}

QFuture<Result> InvidiousApi::subscribeToChannel(QStringView channel)
{
    return post<Result>(authenticatedNetworkRequest(subscribeUrl(channel)), {}, checkIsReplyOk);
}

QFuture<Result> InvidiousApi::unsubscribeFromChannel(QStringView channel)
{
    return deleteResource<Result>(authenticatedNetworkRequest(subscribeUrl(channel)), checkIsReplyOk);
}

Error InvidiousApi::invalidJsonError()
{
    return std::pair(QNetworkReply::InternalServerError, tr("Server returned no valid JSON."));
}

Result InvidiousApi::checkIsReplyOk(QNetworkReply *reply)
{
    auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status >= 200 && status < 300) {
        return Success();
    }
    return std::pair(QNetworkReply::InternalServerError, tr("Server returned status code ") + QString::number(status));
}

QFuture<VideoListResult> InvidiousApi::requestVideoList(VideoListType queryType, QStringView queryValue, qint32 page)
{
    auto url = videoListUrl(queryType, queryValue, page);
    // Feed requests require to be authenticated
    auto request = queryType == Feed ? authenticatedNetworkRequest(std::move(url))
                                     : QNetworkRequest(url);

    return get<VideoListResult>(std::move(request), [=](QNetworkReply *reply) -> VideoListResult {
        if (auto doc = QJsonDocument::fromJson(reply->readAll()); !doc.isNull()) {
            if (queryType == Feed) {
                const auto obj = doc.object();

                // add videos marked as notification
                auto results = VideoBasicInfo::fromJson(obj.value("notifications").toArray());
                for (auto &video : results) {
                    video.setIsNotification(true);
                }

                // add the rest
                results << VideoBasicInfo::fromJson(obj.value("videos").toArray());
                return results;
            }
            return VideoBasicInfo::fromJson(doc.array());
        }
        return invalidJsonError();
    });
}

QNetworkRequest InvidiousApi::authenticatedNetworkRequest(QUrl &&url)
{
    QNetworkRequest request(url);
    if (!m_credentials.isAnonymous()) {
        const QList<QNetworkCookie> cookies {
            m_credentials.cookie().value()
        };
        request.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(cookies));
    }
    // some invidious instances redirect some calls using reverse proxies
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}

QUrlQuery InvidiousApi::genericUrlQuery() const
{
    QUrlQuery query;
    if (!m_language.isEmpty()) {
        query.addQueryItem("hl", m_language);
    }
    if (!m_region.isEmpty()) {
        query.addQueryItem("region", m_region);
    }
    return query;
}

QUrl InvidiousApi::logInUrl() const
{
    QUrl url(invidiousInstance() % API_LOGIN);
    auto urlQuery = genericUrlQuery();
    urlQuery.addQueryItem("referer", QUrl::toPercentEncoding("/"));
    urlQuery.addQueryItem("type", "invidious");
    url.setQuery(urlQuery);
    return url;
}

QUrl InvidiousApi::videoUrl(QStringView videoId) const
{
    return QUrl(invidiousInstance() % API_VIDEOS % u'/' % videoId);
}

QUrl InvidiousApi::videoListUrl(VideoListType queryType, QStringView queryValue, qint32 page) const
{
    auto urlString = invidiousInstance();
    auto query = genericUrlQuery();

    switch (queryType) {
    case Search:
        urlString.append(API_SEARCH);

        query.addQueryItem("q", QUrl::toPercentEncoding(queryValue.toString()));
        query.addQueryItem("page", QString::number(page));
        break;
    case Trending:
        urlString.append(API_TRENDING);

        if (!queryValue.isEmpty()) {
            query.addQueryItem("type", queryValue.toString());
        }
        break;
    case Top:
        urlString.append(API_TOP);
        break;
    case Feed:
        urlString.append(API_FEED);
        break;
    }

    QUrl url(urlString);
    url.setQuery(query);
    return url;
}

QUrl InvidiousApi::subscriptionsUrl() const
{
    auto url = QUrl(invidiousInstance() % API_SUBSCRIPTIONS);
    QUrlQuery query;
    query.addQueryItem("fields", "authorId");
    url.setQuery(query);
    return url;

}

QUrl InvidiousApi::subscribeUrl(QStringView channelId) const
{
    return QUrl(invidiousInstance() % API_SUBSCRIPTIONS % u'/' % channelId);
}
