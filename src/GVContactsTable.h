/*
qgvdial is a cross platform Google Voice Dialer
Copyright (C) 2010  Yuvraaj Kelkar

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Contact: yuvraaj@gmail.com
*/

#ifndef __GVCONTACTSTABLE_H__
#define __GVCONTACTSTABLE_H__

#include "global.h"

// For some reason the symbian MOC doesn't like it if I don't include QObject
// even though it is present in QtCore which is included in global.h
#include <QObject>

class ContactsModel;

class GVContactsTable : public QObject
{
    Q_OBJECT

public:
    GVContactsTable (QObject *parent = 0);
    ~GVContactsTable ();

    void setTempStore(const QString &strTemp);

    void deinitModel ();
    void initModel ();

    //! Use this to set the username and password for the contacts API login
    void setUserPass (const QString &strU, const QString &strP);

    //! Use this to login
    void loginSuccess ();
    //! Use this to logout
    void loggedOut ();

signals:
    //! Status emitter for status bar
    void status(const QString &strText, int timeout = 2000);

    //! Emitted when all contacts are done
    void allContacts (bool bOk);

    //! Emitted when the contacts model is created
    void setContactsModel(QAbstractItemModel *model);

public slots:
    void refreshContacts (const QDateTime &dtUpdate);
    void refreshContacts ();
    void refreshAllContacts ();
    void onSearchQueryChanged (const QString &query);

private slots:
    //! Invoked on response to login to contacts API
    void onLoginResponse (QNetworkReply *reply);
    //! Invoked when the captcha is done
    void onCaptchaDone (bool bOk, const QString &strCaptcha);

    // Invoked when the google contacts API responds with the contacts
    void onGotContacts (QNetworkReply *reply);
    // Invoked when one contact is parsed out of the XML
    void gotOneContact (const ContactInfo &contactInfo);
    //! Invoked when all the contacts are parsed
    void onContactsParsed(bool rv);

    //! Invoked when the contact model tells us that the photo is not present
    void onNoContactPhoto(const ContactInfo &contactInfo);

private:
    QNetworkRequest createRequest(QString strUrl);

    QNetworkReply *
    postRequest (QString         strUrl,
                 QStringPairList arrPairs,
                 QObject        *receiver,
                 const char     *method);
    QNetworkReply *
    getRequest (QString         strUrl,
                QObject        *receiver,
                const char     *method);


private:
    ContactsModel *modelContacts;

    //! Path to the temp directory to store all contact photos
    QString         strTempStore;

    //! Username and password for google authentication
    QString         strUser, strPass;
    //! The authentication string returned by the contacts API
    QString         strGoogleAuth;

    //! The network manager for contacts API
    QNetworkAccessManager nwMgr;

    //! Mutex protecting the following variable
    QMutex          mutex;

    //! Is the user logged in?
    bool            bLoggedIn;

    //! Refresh requested but waiting for login
    bool            bRefreshRequested;

    //! Is the contacts refresh an update process?
    bool            bRefreshIsUpdate;
};

#endif // __GVCONTACTSTABLE_H__