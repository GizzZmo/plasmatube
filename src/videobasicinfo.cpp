/*
 * Copyright 2019  Linus Jahn <lnj@kaidan.im>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "videobasicinfo.h"
#include <QJsonArray>

void VideoThumbnail::parseFromJson(const QJsonObject &obj)
{
    m_quality = obj.value("quality").toString();
    m_url = QUrl(obj.value("url").toString());
    m_width = obj.value("width").toInt();
    m_height = obj.value("height").toInt();
}

QString VideoThumbnail::quality() const
{
    return m_quality;
}

void VideoThumbnail::setQuality(const QString &quality)
{
    m_quality = quality;
}

QUrl VideoThumbnail::url() const
{
    return m_url;
}

void VideoThumbnail::setUrl(const QUrl &url)
{
    m_url = url;
}

qint32 VideoThumbnail::width() const
{
    return m_width;
}

void VideoThumbnail::setWidth(qint32 width)
{
    m_width = width;
}

qint32 VideoThumbnail::height() const
{
    return m_height;
}

void VideoThumbnail::setHeight(qint32 height)
{
    m_height = height;
}

void VideoBasicInfo::parseFromJson(const QJsonObject &obj)
{
    m_videoId = obj.value("videoId").toString();
    m_title = obj.value("title").toString();
    m_length = QTime().addSecs(obj.value("lengthSeconds").toInt());
    m_viewCount = obj.value("viewCount").toInt();
    m_author = obj.value("author").toString();
    m_authorId = obj.value("authorId").toString();
    m_authorUrl = obj.value("authorUrl").toString();
    // FIXME: 2038 problem (timestamp is only 32 bit long)
    m_published.setSecsSinceEpoch(obj.value("published").toInt());
    m_publishedText = obj.value("publishedText").toString();
    m_description = obj.value("description").toString();
    m_descriptionHtml = obj.value("descriptionHtml").toString();
    m_liveNow = obj.value("liveNow").toBool(false);
    m_paid = obj.value("paid").toBool(false);
    m_premium = obj.value("premium").toBool(false);

    m_videoThumbnails = QList<VideoThumbnail>();
    foreach (const QJsonValue &val, obj.value("videoThumbnails").toArray()) {
        VideoThumbnail thumb;
        thumb.parseFromJson(val.toObject());
        m_videoThumbnails.append(thumb);
    }
}

QString VideoBasicInfo::videoId() const
{
    return m_videoId;
}

void VideoBasicInfo::setVideoId(const QString &videoId)
{
    m_videoId = videoId;
}

QString VideoBasicInfo::title() const
{
    return m_title;
}

void VideoBasicInfo::setTitle(const QString &title)
{
    m_title = title;
}

QList<VideoThumbnail> VideoBasicInfo::videoThumbnails() const
{
    return m_videoThumbnails;
}

void VideoBasicInfo::setVideoThumbnails(const QList<VideoThumbnail> &videoThumbnails)
{
    m_videoThumbnails = videoThumbnails;
}

VideoThumbnail VideoBasicInfo::thumbnail(const QString &quality) const
{
    for (auto &thumb : m_videoThumbnails) {
        if (thumb.quality() == quality)
            return thumb;
    }
    return VideoThumbnail();
}

QTime VideoBasicInfo::length() const
{
    return m_length;
}

void VideoBasicInfo::setLength(const QTime &length)
{
    m_length = length;
}

qint64 VideoBasicInfo::viewCount() const
{
    return m_viewCount;
}

void VideoBasicInfo::setViewCount(qint64 viewCount)
{
    m_viewCount = viewCount;
}

QString VideoBasicInfo::author() const
{
    return m_author;
}

void VideoBasicInfo::setAuthor(const QString &author)
{
    m_author = author;
}

QString VideoBasicInfo::authorId() const
{
    return m_authorId;
}

void VideoBasicInfo::setAuthorId(const QString &authorId)
{
    m_authorId = authorId;
}

QString VideoBasicInfo::authorUrl() const
{
    return m_authorUrl;
}

void VideoBasicInfo::setAuthorUrl(const QString &authorUrl)
{
    m_authorUrl = authorUrl;
}

QDateTime VideoBasicInfo::published() const
{
    return m_published;
}

void VideoBasicInfo::setPublished(const QDateTime &published)
{
    m_published = published;
}

QString VideoBasicInfo::publishedText() const
{
    return m_publishedText;
}

void VideoBasicInfo::setPublishedText(const QString &publishedText)
{
    m_publishedText = publishedText;
}

QString VideoBasicInfo::description() const
{
    return m_description;
}

void VideoBasicInfo::setDescription(const QString &description)
{
    m_description = description;
}

QString VideoBasicInfo::descriptionHtml() const
{
    return  m_descriptionHtml;
}

void VideoBasicInfo::setDescriptionHtml(const QString &descriptionHtml)
{
    m_descriptionHtml = descriptionHtml;
}

bool VideoBasicInfo::liveNow() const
{
    return m_liveNow;
}

void VideoBasicInfo::setLiveNow(bool liveNow)
{
    m_liveNow = liveNow;
}

bool VideoBasicInfo::paid() const
{
    return m_paid;
}

void VideoBasicInfo::setPaid(bool paid)
{
    m_paid = paid;
}

bool VideoBasicInfo::premium() const
{
    return m_premium;
}

void VideoBasicInfo::setPremium(bool premium)
{
    m_premium = premium;
}
