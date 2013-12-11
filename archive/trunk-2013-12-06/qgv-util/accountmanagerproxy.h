/*
 * This file was generated by qdbusxml2cpp version 0.7
 * Command line was: qdbusxml2cpp -c AccountManagerProxy -p accountmanagerproxy.h:accountmanagerproxy.cpp org.freedesktop.Telepathy.AccountManager.xml org.freedesktop.Telepathy.AccountManager
 *
 * qdbusxml2cpp is Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef ACCOUNTMANAGERPROXY_H_1281083196
#define ACCOUNTMANAGERPROXY_H_1281083196

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface org.freedesktop.Telepathy.AccountManager
 */
class AccountManagerProxy: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.freedesktop.Telepathy.AccountManager"; }

public:
    AccountManagerProxy(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~AccountManagerProxy();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath> CreateAccount(const QString &Connection_Manager, const QString &Protocol, const QString &Display_Name, const QVariantMap &Parameters, const QVariantMap &Properties)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(Connection_Manager) << qVariantFromValue(Protocol) << qVariantFromValue(Display_Name) << qVariantFromValue(Parameters) << qVariantFromValue(Properties);
        return asyncCallWithArgumentList(QLatin1String("CreateAccount"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void AccountRemoved(const QDBusObjectPath &in0);
    void AccountValidityChanged(const QDBusObjectPath &in0, bool in1);
};

namespace org {
  namespace freedesktop {
    namespace Telepathy {
      typedef ::AccountManagerProxy AccountManager;
    }
  }
}
#endif