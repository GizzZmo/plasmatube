// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "videolistmodel.h"
#include "plasmatube.h"

#include <QFutureWatcher>
#include <QNetworkReply>

QString VideoListModel::queryTypeString(QueryType type)
{
    switch (type) {
    case Feed:
        return tr("Subscriptions");
    case Top:
        return tr("Invidious Top");
    case Trending:
        return tr("Trending");
    case TrendingGaming:
        return tr("Trending Gaming");
    case TrendingMovies:
        return tr("Trending Movies");
    case TrendingMusic:
        return tr("Trending Music");
    case TrendingNews:
        return tr("Trending News");
    default:
        return {};
    }
}

QString VideoListModel::queryTypeIcon(QueryType type)
{
    switch (type) {
    case Feed:
        return QStringLiteral("feed-subscribe");
    case Top:
        return QStringLiteral("arrow-up-double");
    case Trending:
        // should actually be "user-trash-full-symbolic"
        return QStringLiteral("favorite");
    case TrendingGaming:
        return QStringLiteral("folder-games-symbolic");
    case TrendingMovies:
        return QStringLiteral("folder-videos-symbolic");
    case TrendingMusic:
        return QStringLiteral("folder-music-symbolic");
    case TrendingNews:
        return QStringLiteral("message-news");
    default:
        return {};
    }
}

VideoListModel::VideoListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

VideoListModel::VideoListModel(const QList<QInvidious::VideoBasicInfo> &list,
                               QObject *parent)
    : QAbstractListModel(parent),
      m_constant(true),
      m_results(list)
{
}

QHash<int, QByteArray> VideoListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {TitleRole, "title"},
        {ThumbnailRole, "thumbnail"},
        {LengthRole, "length"},
        {ViewCountRole, "viewCount"},
        {AuthorRole, "author"},
        {AuthorIdRole, "authorId"},
        {AuthorUrlRole, "authorUrl"},
        {PublishedRole, "published"},
        {PublishedTextRole, "publishedText"},
        {DescriptionRole, "description"},
        {DescriptionHtmlRole, "descriptionHtml"},
        {LiveNowRole, "liveNow"},
        {PaidRole, "paid"},
        {PremiumRole, "premium"}
    };
}

int VideoListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_results.size();
}

QVariant VideoListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.parent().isValid())
        return {};

    const QInvidious::VideoBasicInfo &video = m_results.at(index.row());
    switch (role) {
    case IdRole:
        return video.videoId();
    case TitleRole:
        return video.title();
    case ThumbnailRole:
        return video.thumbnail("medium").url();
    case LengthRole:
        return video.length();
    case ViewCountRole:
        return video.viewCount();
    case AuthorRole:
        return video.author();
    case AuthorIdRole:
        return video.authorId();
    case AuthorUrlRole:
        return video.authorUrl();
    case PublishedRole:
        return video.published();
    case PublishedTextRole:
        return video.publishedText();
    case DescriptionRole:
        return video.description();
    case DescriptionHtmlRole:
        return video.descriptionHtml();
    case LiveNowRole:
        return video.liveNow();
    case PaidRole:
        return video.paid();
    case PremiumRole:
        return video.premium();
    }
    return {};
}

void VideoListModel::fetchMore(const QModelIndex &index)
{
    if (canFetchMore(index)) {
        auto future = PlasmaTube::instance().api()->requestSearchResults(m_searchQuery, ++m_currentPage);
        handleQuery(future, Search, false);
    }
}

bool VideoListModel::canFetchMore(const QModelIndex &) const
{
    return !m_futureWatcher && m_queryType == Search;
}

bool VideoListModel::isLoading() const
{
    return m_futureWatcher != nullptr;
}

QString VideoListModel::title() const
{
    switch (m_queryType) {
    case Search:
        return tr("Search results for \"%1\"").arg(m_searchQuery);
    default:
        return queryTypeString(m_queryType);
    }
}

void VideoListModel::requestSearchResults(const QString &searchString)
{
    m_searchQuery = searchString;
    m_currentPage = 0;
    handleQuery(PlasmaTube::instance().api()->requestSearchResults(searchString), Search);
}

void VideoListModel::requestQuery(QueryType type)
{
    m_searchQuery.clear();
    switch (type) {
    case Feed:
        handleQuery(PlasmaTube::instance().api()->requestFeed(), type);
        break;
    case Top:
        handleQuery(PlasmaTube::instance().api()->requestTop(), type);
        break;
    case Trending:
        handleQuery(PlasmaTube::instance().api()->requestTrending(), type);
        break;
    case TrendingGaming:
        handleQuery(PlasmaTube::instance().api()->requestTrending(QInvidious::Gaming), type);
        break;
    case TrendingMovies:
        handleQuery(PlasmaTube::instance().api()->requestTrending(QInvidious::Movies), type);
        break;
    case TrendingMusic:
        handleQuery(PlasmaTube::instance().api()->requestTrending(QInvidious::Music), type);
        break;
    case TrendingNews:
        handleQuery(PlasmaTube::instance().api()->requestTrending(QInvidious::News), type);
        break;
    default:
        qDebug() << "VideoListModel::requestQuery() called with not allowed type" << type;
    }
}

void VideoListModel::refresh()
{
    if (!isLoading() && m_queryType != NoQuery) {
        switch (m_queryType) {
        case Search:
            requestSearchResults(m_searchQuery);
            break;
        default:
            requestQuery(m_queryType);
        }
    }
}

void VideoListModel::handleQuery(QFuture<QInvidious::VideoListResult> future, QueryType type, bool reset)
{
    // reset loaded videos (if requested)
    if (reset) {
        clearAll();
    }

    // stop running task
    if (m_futureWatcher) {
        // TODO: cancelling isn't implemented yet
        m_futureWatcher->cancel();
        m_futureWatcher->deleteLater();
        m_futureWatcher = nullptr;
    }

    // set up new task
    m_futureWatcher = new QFutureWatcher<QInvidious::VideoListResult>();
    connect(m_futureWatcher, &QFutureWatcherBase::finished, this, [this] {
        auto result = m_futureWatcher->result();
        if (auto videos = std::get_if<QList<QInvidious::VideoBasicInfo>>(&result)) {
            const auto rows = rowCount();
            beginInsertRows({}, rows, rows + videos->size() - 1);
            m_results << *videos;
            endInsertRows();
        } else if (auto error = std::get_if<QInvidious::Error>(&result)) {
            emit errorOccured(error->second);
        }

        m_futureWatcher->deleteLater();
        m_futureWatcher = nullptr;
        emit isLoadingChanged();
    });

    setQueryType(type);
    m_futureWatcher->setFuture(future);
    emit isLoadingChanged();
}

void VideoListModel::setQueryType(QueryType type)
{
    // title changes if the type changes or the search string has been updated (page 0)
    if (m_queryType != type || m_searchQuery == 0) {
        m_queryType = type;
        emit titleChanged();
    }
}

void VideoListModel::clearAll()
{
    beginResetModel();
    m_results.clear();
    endResetModel();
}
