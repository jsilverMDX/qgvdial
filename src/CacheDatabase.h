#ifndef __CACHEDATABASE_H__
#define __CACHEDATABASE_H__

#include "global.h"

// For some reason the symbian MOC doesn't like it if I don't include QObject
// even though it is present in QtCore which is included in global.h
#include <QObject>

class ContactsModel;
class InboxModel;

class CacheDatabase : public QObject
{
    Q_OBJECT

private:
    CacheDatabase(const QSqlDatabase &other, QObject *parent = 0);
    ~CacheDatabase(void);

public:
    void init ();
    void deinit ();

    // Toggle quick and dirty!
    void setQuickAndDirty(bool bBeDirty = true);

    // Contacts model
    ContactsModel *newContactsModel();
    void clearContacts ();
    void refreshContactsModel (ContactsModel *modelContacts);

    // username and password
    bool getUserPass (QString &strUser, QString &strPass);
    bool putUserPass (const QString &strUser, const QString &strPass);

    // GV callback / callout method
    bool getCallback (QString &strCallback);
    bool putCallback (const QString &strCallback);

    // GV Registered numbers
    bool getRegisteredNumbers (GVRegisteredNumberArray &listNumbers);
    bool putRegisteredNumbers (const GVRegisteredNumberArray &listNumbers);

    // Single contact based on contact identifier
    bool existsContact (const QString &strId);
    bool deleteContact (const QString &strId);
    bool insertContact (const ContactInfo &info);
    quint32 getContactsCount ();

    // Contact information based on contact identifier
    bool getContactFromLink (ContactInfo &info);
    bool getContactFromNumber (const QString &strNumber, ContactInfo &info);

    // Last update of contacts
    void clearLastContactUpdate ();
    bool setLastContactUpdate (const QDateTime &dateTime);
    bool getLastContactUpdate (QDateTime &dateTime);

    // Inbox model
    InboxModel *newInboxModel();
    void clearInbox ();
    void refreshInboxModel (InboxModel *modelInbox,
                            const QString &strType);
    quint32 getInboxCount (GVI_Entry_Type Type);

    // Last update of inbox
    bool setLastInboxUpdate (const QDateTime &dateTime);
    bool getLastInboxUpdate (QDateTime &dateTime);
    bool getLatestInboxEntry (QDateTime &dateTime);

    // Single inbox entry
    bool existsInboxEntry (const GVInboxEntry &hEvent);
    bool insertInboxEntry (const GVInboxEntry &hEvent);

    // proxy settings get and set
    bool setProxySettings (bool bEnable,
                           bool bUseSystemProxy,
                           const QString &host, int port,
                           bool bRequiresAuth,
                           const QString &user, const QString &pass);
    bool getProxySettings (bool &bEnable,
                           bool &bUseSystemProxy,
                           QString &host, int &port,
                           bool &bRequiresAuth,
                           QString &user, QString &pass);

    // The GV Inbox that is being displayed
    bool getInboxSelector (QString &strSelector);
    bool putInboxSelector (const QString &strSelector);

    // Mosquitto settings get and set
    bool setMqSettings (bool bEnable, const QString &host, int port,
                        const QString &topic);
    bool getMqSettings (bool &bEnable, QString &host, int &port,
                        QString &topic);

private:
    bool putContactInfo (const ContactInfo &info);
    bool deleteContactInfo (const QString &strId);

signals:
    void status(const QString &strText, int timeout = 2000);

private:
    QSqlDatabase    dbMain;

    friend class Singletons;
};

#endif //__CACHEDATABASE_H__
