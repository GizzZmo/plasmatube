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

#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QObject>
#include <QNetworkCookie>
#include <QVector>

class QNetworkAccessManager;
class QNetworkRequest;

class AccountManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
    Q_PROPERTY(QString invidiousId READ invidiousId NOTIFY invidiousIdChanged)
    Q_PROPERTY(QVector<QString> subscribedChanneldIds READ subscribedChanneldIds NOTIFY subscribedChannelIdsChanged)

public:
    AccountManager(QObject *parent = nullptr);
    ~AccountManager();

    static AccountManager *instance();

    Q_INVOKABLE void logIn(const QString &username, const QString &password);
    Q_INVOKABLE void logOut();

    Q_INVOKABLE void fetchSubscriptions();
    Q_INVOKABLE void subscribeToChannel(const QString &channelId);
    Q_INVOKABLE void unsubscribeFromChannel(const QString &channelId);

    QString username() const;
    QString invidiousId() const;
    QString invidiousInstance() const;
    QNetworkCookie cookie() const;

    QVector<QString> subscribedChanneldIds() const;

    QNetworkAccessManager *netManager();
    static QNetworkRequest networkRequestWithCookie(const QString &url);

signals:
    void loggedIn();
    void loggingInFailed(const QString &errorText);

    void loggedOut();

    void subscribedToChannel(const QString &channelId);
    void subscribingFailed(const QString &channelId, const QString &errorText);
    void unsubscribedFromChannel(const QString &channelId);
    void unsubscribingFailed(const QString &channelId, const QString &errorText);

    void usernameChanged();
    void invidiousIdChanged();

    void subscribedChannelIdsChanged();

private:
    void setSubscribedChannelIds(const QVector<QString> &);

    void saveCredentials();
    
    QNetworkAccessManager *m_netManager;
    QString m_username;
    QString m_instance;
    QNetworkCookie m_cookie;

    QVector<QString> m_subscribedChannelIds;
};

#endif // ACCOUNTMANAGER_H
