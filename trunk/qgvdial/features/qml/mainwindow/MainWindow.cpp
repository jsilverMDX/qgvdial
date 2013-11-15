/*
qgvdial is a cross platform Google Voice Dialer
Copyright (C) 2009-2013  Yuvraaj Kelkar

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

#include "MainWindow.h"
#include "QmlView.h"

#ifdef Q_OS_BLACKBEERRY
#include <QGLWidget>
#include <QGLFormat>
#endif

#ifndef UNKNOWN_CONTACT_QRC_PATH
#error Must define the unknown contact QRC path
#endif

#ifndef USE_SINGLE_APPLICATION
#error Must define USE_SINGLE_APPLICATION macro in your platform_specifics.h
#endif

#if USE_SINGLE_APPLICATION
#include "QtSingleApplication"
#endif

#include "ContactsModel.h"
#include "InboxModel.h"
#include "GVNumModel.h"
#include "ContactNumbersModel.h"

QApplication *
createAppObject(int &argc, char **argv)
{
#if USE_SINGLE_APPLICATION
    QtSingleApplication *app;

    app = new QtSingleApplication(argc, argv);
    if (NULL == app) {
        Q_WARN("Failed to create QtSingleApplication object");
        return app;
    }

    if (app->isRunning ()) {
        app->sendMessage ("show");
        delete app;
        app = NULL;
    } else {
        app->setQuitOnLastWindowClosed (false);
    }
#else
    QApplication *app = createApplication(argc, argv);
    if (NULL == app) {
        Q_WARN("Failed to create QApplication object");
        return app;
    }

    app->setQuitOnLastWindowClosed (false);
#endif

    return app;
}//createAppObject

MainWindow::MainWindow(QObject *parent)
: IMainWindow(parent)
, m_view(new QmlView)
, mainPageStack(NULL)
, mainTabGroup(NULL)
, loginExpand(NULL)
, loginButton(NULL)
, tfaPinDlg(NULL)
, textUsername(NULL)
, textPassword(NULL)
, infoBanner(NULL)
, appPwDlg(NULL)
, contactsList(NULL)
, inboxList(NULL)
, inboxSelector(NULL)
, proxySettingsPage(NULL)
, selectedNumberButton(NULL)
, regNumberSelector(NULL)
, ciSelector(NULL)
, statusBanner(NULL)
, dialPage(NULL)
{
#ifdef Q_OS_BLACKBEERRY
    // GL viewport increases performance on blackberry
    QGLFormat format = QGLFormat::defaultFormat();
    format.setSampleBuffers(false);
    QGLWidget *glWidget = new QGLWidget(format);
    glWidget->setAutoFillBackground(false);

    m_view->setViewport(glWidget);

    // More gfx performance
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setAttribute(Qt::WA_OpaquePaintEvent);
    m_view->setAttribute(Qt::WA_NoSystemBackground);
    m_view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    m_view->viewport()->setAttribute(Qt::WA_NoSystemBackground);
#endif
}//MainWindow::MainWindow

MainWindow::~MainWindow()
{
    if (NULL != m_view) {
        delete m_view;
        m_view = NULL;
    }
}//MainWindow::~MainWindow

void
MainWindow::init()
{
    IMainWindow::init ();

    bool rv =
    connect(m_view, SIGNAL(statusChanged(QDeclarativeView::Status)),
            this, SLOT(declStatusChanged(QDeclarativeView::Status)));
    Q_ASSERT(rv);
    if (!rv) {
        Q_WARN("Failed to connect to declStatusChanged signal");
        qApp->quit ();
        exit(-1);
        return;
    }

    m_view->setOrientation(QmlApplicationViewer::ScreenOrientationAuto);
    m_view->setMainQmlFile(QLatin1String(MAIN_QML_PATH));
    m_view->showExpanded();

#if USE_SINGLE_APPLICATION
    rv = connect(qApp, SIGNAL(messageReceived(QString)),
                 this, SLOT(messageReceived(QString)));
    if (!rv) {
        Q_WARN("Failed to connect to message received signal");
        qApp->quit ();
        exit(-1);
        return;
    }
#endif
}//MainWindow::init

QObject *
MainWindow::getQMLObject(const char *pageName)
{
    QObject *pObj = NULL;
    do { // Begin cleanup block (not a loop)
        QObject *pRoot = (QObject *) m_view->rootObject ();
        if (NULL == pRoot) {
            Q_WARN(QString("Couldn't get root object in QML for %1")
                    .arg(pageName));
            break;
        }

        if (pRoot->objectName() == pageName) {
            pObj = pRoot;
            break;
        }

        pObj = pRoot->findChild <QObject*> (pageName);
        if (NULL == pObj) {
            Q_WARN(QString("Could not find page %1").arg (pageName));
            break;
        }
    } while (0); // End cleanup block (not a loop)

    return (pObj);
}//MainWindow::getQMLObject

void
MainWindow::log(QDateTime /*dt*/, int /*level*/, const QString & /*strLog*/)
{
    //TODO: Show it in the logs view
}//MainWindow::log

void
MainWindow::declStatusChanged(QDeclarativeView::Status status)
{
    do {
        if (QDeclarativeView::Ready != status) {
            Q_WARN(QString("status = %1").arg (status));
            break;
        }

        mainPageStack = getQMLObject ("MainPageStack");
        if (NULL == mainPageStack) {
            break;
        }
        connect(mainPageStack, SIGNAL(sigShowContact(QString)),
                &oContacts, SLOT(getContactInfoAndModel(QString)));
        connect(mainPageStack, SIGNAL(sigRefreshContacts()),
                &oContacts, SLOT(refreshLatest()));
        connect(mainPageStack, SIGNAL(sigRefreshInbox()),
                &oInbox, SLOT(refreshLatest()));

        mainTabGroup = getQMLObject ("MainTabGroup");
        if (NULL == mainTabGroup) {
            break;
        }

        loginExpand = getQMLObject ("ExpandLoginDetails");
        if (NULL == loginExpand) {
            break;
        }

        loginButton = getQMLObject ("LoginButton");
        if (NULL == loginButton) {
            break;
        }
        connect(loginButton, SIGNAL(clicked()),
                this, SLOT(onLoginButtonClicked()));

        tfaPinDlg = getQMLObject ("TFAPinDialog");
        if (NULL == tfaPinDlg) {
            break;
        }
        connect(tfaPinDlg, SIGNAL(done(bool)), this, SLOT(onTfaPinDlg(bool)));

        textUsername = getQMLObject ("TextUsername");
        if (NULL == textUsername) {
            break;
        }

        textPassword = getQMLObject ("TextPassword");
        if (NULL == textPassword) {
            break;
        }

        infoBanner = getQMLObject ("InfoBanner");
        if (NULL == infoBanner) {
            break;
        }

        contactsList = getQMLObject ("ContactsList");
        if (NULL == contactsList) {
            break;
        }
        connect(contactsList, SIGNAL(contactClicked(QString)),
                &oContacts, SLOT(getContactInfoAndModel(QString)));

        inboxList = getQMLObject ("InboxList");
        if (NULL == inboxList) {
            break;
        }
        connect(inboxList, SIGNAL(clicked(QString)),
                this, SLOT(onInboxClicked(QString)));

        inboxSelector = getQMLObject ("InboxSelector");
        if (NULL == inboxSelector) {
            break;
        }
        connect(inboxSelector, SIGNAL(selectionChanged(QString)),
                this, SLOT(onInboxSelectionChanged(QString)));

        appPwDlg = getQMLObject ("AppPwDialog");
        if (NULL == appPwDlg) {
            break;
        }
        connect(appPwDlg, SIGNAL(done(bool)), this, SLOT(onAppPwDlg(bool)));

        proxySettingsPage = getQMLObject ("ProxySettingsPage");
        if (NULL == proxySettingsPage) {
            break;
        }
        connect(proxySettingsPage,
                SIGNAL(sigProxyChanges(bool,bool,QString,int,bool,QString,QString)),
                this,
                SLOT(onSigProxyChanges(bool,bool,QString,int,bool,QString,QString)));
        connect(proxySettingsPage, SIGNAL(sigRevertChanges()),
                this, SLOT(onUserProxyRevert()));

        selectedNumberButton = getQMLObject ("SelectedNumberButton");
        if (NULL == selectedNumberButton) {
            break;
        }

        regNumberSelector = getQMLObject ("RegNumberSelector");
        if (NULL == regNumberSelector) {
            break;
        }
        connect (regNumberSelector, SIGNAL(selected(QString)),
                 &oPhones, SLOT(onUserSelectPhone(QString)));
        connect (regNumberSelector, SIGNAL(modify(QString)),
                 &oPhones, SLOT(onUserUpdateCiNumber(QString)));

        ciSelector = getQMLObject ("CiPhoneSelectionPage");
        if (NULL == ciSelector) {
            break;
        }
        connect(ciSelector, SIGNAL(setCiNumber(QString,QString)),
                &oPhones, SLOT(linkCiToNumber(QString,QString)));

        statusBanner = getQMLObject ("StatusBanner");
        if (NULL == statusBanner) {
            break;
        }

        dialPage = getQMLObject ("DialPage");
        if (NULL == dialPage) {
            break;
        }
        connect(dialPage, SIGNAL(sigCall(QString)),
                this, SLOT(onUserCall(QString)));
        //connect(dialPage, SIGNAL(sigText(QString)),
        //        this, SLOT(onUserText(QString)));

        onInitDone();
        return;
    } while(0);
    exit(-1);
}//MainWindow::declStatusChanged

void
MainWindow::messageReceived(const QString &msg)
{
    if (msg == "show") {
        Q_DEBUG ("Second instance asked us to show");
        m_view->show ();
    } else if (msg == "quit") {
        Q_DEBUG ("Second instance asked us to quit");
        qApp->quit ();
    }
}//MainWindow::messageReceived

void
MainWindow::uiUpdateProxySettings(const ProxyInfo &info)
{
    QMetaObject::invokeMethod (proxySettingsPage, "setValues",
                               Q_ARG (QVariant, QVariant(info.enableProxy)),
                               Q_ARG (QVariant, QVariant(info.useSystemProxy)),
                               Q_ARG (QVariant, QVariant(info.server)),
                               Q_ARG (QVariant, QVariant(info.port)),
                               Q_ARG (QVariant, QVariant(info.authRequired)),
                               Q_ARG (QVariant, QVariant(info.user)),
                               Q_ARG (QVariant, QVariant(info.pass)));
}//MainWindow::uiUpdateProxySettings

void
MainWindow::onSigProxyChanges(bool enable, bool useSystemProxy, QString server,
                              int port, bool authRequired, QString user,
                              QString pass)
{
    ProxyInfo info;
    info.enableProxy    = enable;
    info.useSystemProxy = useSystemProxy;
    info.server         = server;
    info.port           = port;
    info.authRequired   = authRequired;
    info.user           = user;
    info.pass           = pass;

    onUiProxyChanged (info);
}//MainWindow::onSigProxyChanges

void
MainWindow::onLoginButtonClicked()
{
    if ("Login" == loginButton->property("text").toString()) {
        QString user, pass;
        user = textUsername->property("text").toString();
        pass = textPassword->property("text").toString();

        beginLogin (user, pass);
    } else {
        onUserLogoutRequest ();
    }
}//MainWindow::onLoginButtonClicked

void
MainWindow::onUserLogoutDone()
{
    Q_DEBUG("Logout complete");
}//MainWindow::onUserLogoutDone

void
MainWindow::uiRequestLoginDetails()
{
    // Show the settings tab
    QMetaObject::invokeMethod (mainTabGroup, "setTab", Q_ARG(QVariant, 3));
    // Show login settings if it isn't already shown.
    if (!loginExpand->property("isExpanded").toBool()) {
        bool val = true;
        loginExpand->setProperty("isExpanded", val);
    }
}//MainWindow::uiRequestLoginDetails

void
MainWindow::uiRequestTFALoginDetails(void *ctx)
{
    loginCtx = ctx;

    // Push the TFA dialog on to the main page
    QMetaObject::invokeMethod (mainPageStack, "pushTfaDlg");
}//MainWindow::uiRequestTFALoginDetails

void
MainWindow::onTfaPinDlg(bool accepted)
{
    QString strPin = tfaPinDlg->property("textPin").toString();
    int pin = strPin.toInt();

    if (accepted) {
        if (pin == 0) {
            resumeTFAAuth (loginCtx, pin, true);
        } else {
            resumeTFAAuth (loginCtx, pin, false);
        }
    }
}//MainWindow::onTfaPinDlg

void
MainWindow::uiSetUserPass(bool editable)
{
    textUsername->setProperty ("text", m_user);
    textPassword->setProperty ("text", m_pass);

    int val = editable ? 1 : 0;
    textUsername->setProperty ("opacity", val);
    textPassword->setProperty ("opacity", val);

    loginButton->setProperty ("text", editable ? "Login" : "Logout");
}//MainWindow::uiSetUserPass

void
MainWindow::uiRequestApplicationPassword()
{
    QMetaObject::invokeMethod (mainPageStack, "pushAppPwDlg");
}//MainWindow::uiRequestApplicationPassword

void
MainWindow::onAppPwDlg(bool accepted)
{
    if (accepted) {
        QString appPw = appPwDlg->property("appPw").toString();
        oContacts.login (m_user, appPw);
    }
}//MainWindow::onAppPwDlg

void
MainWindow::uiLoginDone(int status, const QString &errStr)
{
    if (ATTS_SUCCESS == status) {
        return;
    }

    QString msg;
    if (ATTS_NW_ERROR == status) {
        msg = "Network error. Try again later.";
    } else if (ATTS_USER_CANCEL == status) {
        msg = "User canceled login.";
    } else {
        msg = QString("Login failed: %1").arg (errStr);
    }

    infoBanner->setProperty ("text", msg);
    QMetaObject::invokeMethod (infoBanner, "show");
}//MainWindow::uiLoginDone

void
MainWindow::uiRefreshContacts()
{
    Q_ASSERT(NULL != oContacts.m_contactsModel);

    m_view->engine()->rootContext()
                    ->setContextProperty("g_ContactsModel",
                                         oContacts.m_contactsModel);
    QMetaObject::invokeMethod (contactsList, "setMyModel");
}//MainWindow::uiRefreshContacts

void
MainWindow::uiRefreshInbox()
{
    m_view->engine()->rootContext()
                    ->setContextProperty("g_InboxModel",
                                         oInbox.m_inboxModel);
    QMetaObject::invokeMethod (inboxList, "setMyModel");
}//MainWindow::uiRefreshInbox

void
MainWindow::uiSetSelelctedInbox(const QString &selection)
{
    QMetaObject::invokeMethod (inboxSelector, "setSelection",
                               Q_ARG(QVariant, QVariant(selection)));
}//MainWindow::uiSetSelelctedInbox

void
MainWindow::uiSetNewRegNumbersModel()
{
    m_view->engine()->rootContext()->setContextProperty("g_RegNumberModel",
                                                        oPhones.m_numModel);
    QMetaObject::invokeMethod (regNumberSelector, "setMyModel");
}//MainWindow::uiSetNewRegNumbersModel

void
MainWindow::uiRefreshNumbers()
{
    Q_ASSERT(NULL != oPhones.m_numModel);
    if (NULL == oPhones.m_numModel) {
        Q_CRIT("m_numModel is NULL!");
        return;
    }

    GVRegisteredNumber num;
    if (!oPhones.m_numModel->getSelectedNumber (num)) {
        Q_WARN("No selected number!!");
        return;
    }

    QString btnText = QString("%1 (%2)").arg(num.name, num.number);
    selectedNumberButton->setProperty ("text", btnText);
}//MainWindow::uiRefreshNumbers

void
MainWindow::uiSetNewContactDetailsModel()
{
    m_view->engine()->rootContext()
                    ->setContextProperty("g_ContactPhonesModel",
                                         oContacts.m_contactPhonesModel);
}//MainWindow::uiSetNewContactDetailsModel

void
MainWindow::uiShowContactDetails(const ContactInfo &cinfo)
{
    QMetaObject::invokeMethod (mainPageStack, "showContactDetails",
                               Q_ARG (QVariant, QVariant(cinfo.strPhotoPath)),
                               Q_ARG (QVariant, QVariant(cinfo.strTitle)));
}//MainWindow::uiShowContactDetails

void
MainWindow::onInboxClicked(QString id)
{
    GVInboxEntry event;
    QString type;
    ContactInfo cinfo;

    event.id = id;
    if (!oInbox.getEventInfo (event, cinfo, type)) {
        //TODO: Show error message
        return;
    }

    if (!event.bRead) {
        oInbox.markEntryAsRead (event);
    }

    QMetaObject::invokeMethod(mainPageStack, "showInboxDetails",
                              Q_ARG(QVariant,QVariant(cinfo.strPhotoPath)),
                              Q_ARG(QVariant,QVariant(event.strDisplayNumber)),
                              Q_ARG(QVariant,QVariant(event.strPhoneNumber)),
                              Q_ARG(QVariant,QVariant(event.strNote)),
                              Q_ARG(QVariant,QVariant(event.strText)),
                              Q_ARG(QVariant,QVariant(type)),
                              Q_ARG(QVariant,QVariant(cinfo.strId)));
}//MainWindow::onInboxClicked

void
MainWindow::onInboxSelectionChanged(QString sel)
{
    oInbox.onUserSelect (sel);
}//MainWindow::onInboxSelectionChanged

void
MainWindow::uiGetCIDetails(GVRegisteredNumber &num, GVNumModel *model)
{
    m_view->engine()->rootContext()
                    ->setContextProperty("g_CiPhonesModel", model);

    QMetaObject::invokeMethod (mainPageStack, "pushCiSelector",
                               Q_ARG (QVariant, QVariant(num.id)));
}//MainWindow::uiGetCIDetails

void
MainWindow::uiLongTaskBegins()
{
    QMetaObject::invokeMethod(statusBanner, "showMessage",
                              Q_ARG(QVariant,
                                    QVariant(m_taskInfo.suggestedStatus)),
                              Q_ARG(QVariant,
                                    QVariant(m_taskInfo.suggestedMillisconds)));
}//MainWindow::uiLongTaskBegins

void
MainWindow::uiLongTaskContinues()
{
    QMetaObject::invokeMethod(statusBanner, "showMessage",
                              Q_ARG(QVariant,
                                    QVariant(m_taskInfo.suggestedStatus)),
                              Q_ARG(QVariant,
                                    QVariant(m_taskInfo.suggestedMillisconds)));
}//MainWindow::uiLongTaskContinues

void
MainWindow::uiLongTaskEnds()
{
    QMetaObject::invokeMethod (statusBanner, "clearMessage");
}//MainWindow::uiLongTaskEnds