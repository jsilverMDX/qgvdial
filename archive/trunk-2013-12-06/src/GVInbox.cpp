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

#include "global.h"
#include "GVInbox.h"
#include "Singletons.h"
#include "InboxModel.h"
#include "MainWindow.h"

GVInbox::GVInbox (MainWindow *parent)
: QObject (parent)
, mutex (QMutex::Recursive)
, bLoggedIn (false)
, bRefreshInProgress (false)
, bRetrieveTrash (false)
, modelInbox (NULL)
, bRecentCheckInProgress(false)
{
    bool rv =
    connect (&parent->gvApi,
             SIGNAL(oneInboxEntry(AsyncTaskToken*,GVInboxEntry)),
             this,
             SLOT(oneInboxEntry(AsyncTaskToken*,GVInboxEntry)));
    Q_ASSERT(rv); Q_UNUSED(rv);

    // Initially, all are to be selected
    strSelectedMessages = "all";
}//GVInbox::GVInbox

GVInbox::~GVInbox(void)
{
    deinitModel ();
}//GVInbox::~GVInbox

void
GVInbox::deinitModel ()
{
    if (NULL != modelInbox) {
        delete modelInbox;
        modelInbox = NULL;
    }

    emit setInboxModel (NULL);
}//GVInbox::deinitModel

void
GVInbox::initModel ()
{
    deinitModel ();

    CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
    modelInbox = dbMain.newInboxModel ();

    QString strSelector;
    if (dbMain.getInboxSelector (strSelector)) {
        this->strSelectedMessages = strSelector;
    } else {
        this->strSelectedMessages = "all";
    }

    emit setInboxModel (modelInbox);

    prepView ();
}//GVInbox::initModel

void
GVInbox::prepView ()
{
    if (modelInbox == NULL) {
        return;
    }

    emit status ("Re-selecting inbox entries. This will take some time", 0);
    modelInbox->refresh (strSelectedMessages);
    emit status ("Inbox entries selected.");

    CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
    dbMain.putInboxSelector(this->strSelectedMessages);

    QString strSend = this->strSelectedMessages[0].toUpper()
                    + this->strSelectedMessages.mid (1);

    emit setInboxSelector(strSend);
}//GVInbox::prepView

void
GVInbox::refresh (const QDateTime &dtUpdate)
{
    QMutexLocker locker(&mutex);
    if (!bLoggedIn) {
        return;
    }

    if (bRefreshInProgress) {
        if ((!dtUpdate.isValid()) || (dtUpdate > dateWaterLevel)) {
            dateWaterLevel = dtUpdate;
        }

        Q_WARN("Refresh in progress. Ignore this one.");
        return;
    }

    CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
    QDateTime latest;
    dbMain.getLatestInboxEntry (latest);

    if ((!dateWaterLevel.isValid()) || (dateWaterLevel < latest)) {
        dateWaterLevel = latest;
    }
    if ((!dtUpdate.isValid()) || (dtUpdate > dateWaterLevel)) {
        dateWaterLevel = dtUpdate;
    }

    int page = 1;
    AsyncTaskToken *token = new AsyncTaskToken(this);
    token->inParams["type"] = "all";
    token->inParams["page"] = page;
    passedWaterLevel = false;

    Q_DEBUG(QString ("Water level = %1").arg(dateWaterLevel.toString()));

    bool rv = connect(token, SIGNAL(completed()), this, SLOT(getInboxDone()));
    Q_ASSERT(rv); Q_UNUSED(rv);

    bRefreshInProgress = true;
    newEntries = 0;

    MainWindow *mainWin = (MainWindow *) parent();
    if (!mainWin->gvApi.getInbox (token)) {
        cleanupAfterInboxDone (token);
    }
}//GVInbox::refresh

void
GVInbox::refresh ()
{
    CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
    QDateTime dtUpdate;
    dbMain.getLatestInboxEntry (dtUpdate);

    refresh(dtUpdate);
}//GVInbox::refresh

void
GVInbox::refreshFullInbox ()
{
    QDateTime dtUpdate;
    refresh(dtUpdate);
}//GVInbox::refreshFullInbox

void
GVInbox::checkRecent()
{
    if (bRecentCheckInProgress) {
        Q_WARN("Current check inbox in process");
        return;
    }

    AsyncTaskToken *token = new AsyncTaskToken(this);
    if (NULL == token) {
        Q_WARN("Failed to allocate task to check for recent");
        return;
    }

    bool rv = connect(token, SIGNAL(completed()),
                      this, SLOT(onCheckRecentCompleted()));
    Q_ASSERT(rv); Q_UNUSED(rv);

    MainWindow *mainWin = (MainWindow *) parent ();
    if (!mainWin->gvApi.checkRecentInbox (token)) {
        delete token;
    } else {
        bRecentCheckInProgress = true;
    }
}//GVInbox::checkRecent

void
GVInbox::onCheckRecentCompleted()
{
    AsyncTaskToken *token = (AsyncTaskToken *) QObject::sender ();

    do { // Begin cleanup block (not a loop)
        if (!token) {
            Q_WARN("No token provided. Failure!!");
            break;
        }

        QDateTime latestOnServer = token->outParams["serverLatest"].toDateTime();

        CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
        QDateTime latestInCache;
        dbMain.getLatestInboxEntry (latestInCache);

        if (latestOnServer > latestInCache) {
            refresh ();
        }
    } while (0); // End cleanup block (not a loop)

    if (token) {
        delete token;
        token = NULL;
    }

    bRecentCheckInProgress = false;
}//GVInbox::onCheckRecentCompleted

void
GVInbox::oneInboxEntry (AsyncTaskToken * /*token*/, const GVInboxEntry &hevent)
{
    if (!bRetrieveTrash) {
        if (GVIE_Unknown == hevent.Type) {
            Q_WARN("Invalid inbox entry type:")
                    << QString("%1").arg((int)hevent.Type);
            return;
        }

        if (dateWaterLevel.isValid () && (dateWaterLevel > hevent.startTime)) {
            passedWaterLevel = true;
        } else {
            newEntries++;
        }

        CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
        dbMain.setQuickAndDirty ();

        modelInbox->insertEntry (hevent);
    } else {
        //Q_DEBUG(QString("Delete entry with ID %1").arg (hevent.id));
        passedWaterLevel = true;

        modelInbox->deleteEntry (hevent);
    }
}//GVInbox::oneInboxEntry

void
GVInbox::getInboxDone ()
{
    AsyncTaskToken *token = (AsyncTaskToken *) QObject::sender ();

    bool nextpage = false;
    bool ok = false;
    do { // Begin cleanup block (not a loop)
        if (!token) {
            Q_WARN("No token provided. Failure!!");
            break;
        }

        if (ATTS_SUCCESS != token->status) {
            Q_WARN("Inbox fetch failed for page")
                << token->inParams["page"].toString();
            break;
        }

        if (passedWaterLevel) {
            Q_DEBUG("Started getting old entries");
            ok = true;
            break;
        }

        int messageCount = token->outParams["message_count"].toUInt();
        if (messageCount == 0) {
            Q_DEBUG("No more pages to process.");
            ok = true;
            break;
        }

        int page = token->inParams["page"].toInt(&ok);
        if (!ok) {
            Q_WARN("Invalid page number");
            break;
        }
        if (page >= 30) {
            Q_DEBUG("Page limit reached");
            break;
        }

        page++;

        token->inParams["page"] = page;
        MainWindow *mainWin = (MainWindow *) parent ();
        if (!mainWin->gvApi.getInbox (token)) {
            Q_WARN("Failed to get inbox!");
            break;
        }

        nextpage = true;
    } while (0); // End cleanup block (not a loop)

    if (nextpage) {
        return;
    }

    cleanupAfterInboxDone (token);
}//GVInbox::getInboxDone

void
GVInbox::cleanupAfterInboxDone(AsyncTaskToken *token)
{
    QMutexLocker locker(&mutex);
    bRefreshInProgress = false;

    if (bRetrieveTrash) {
        bRetrieveTrash = false;

        CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
        dbMain.setQuickAndDirty (false);

        prepView ();

        emit status (QString("Inbox ready. %1 new %2 retrieved.")
                     .arg(newEntries).arg (newEntries == 1?"entry":"entries"));
    } else {
        bRetrieveTrash = true;
        getTrash ();
    }

    if (token) {
        token->deleteLater ();
    }
}//GVInbox::cleanupAfterInboxDone

void
GVInbox::getTrash()
{
    int page = 1;
    AsyncTaskToken *token = new AsyncTaskToken(this);
    token->inParams["type"] = "trash";
    token->inParams["page"] = page;
    passedWaterLevel = false;

    bool rv = connect(token, SIGNAL(completed()), this, SLOT(getInboxDone()));
    Q_ASSERT(rv); Q_UNUSED(rv);

    bRefreshInProgress = true;
    newEntries = 0;

    MainWindow *mainWin = (MainWindow *) parent ();
    if (!mainWin->gvApi.getInbox (token)) {
        cleanupAfterInboxDone (token);
    }
}//GVInbox::getTrash

void
GVInbox::onInboxSelected (const QString &strSelection)
{
    QMutexLocker locker(&mutex);
    if (!bLoggedIn) {
        return;
    }

    strSelectedMessages = strSelection.toLower ();
    prepView ();
}//GVInbox::onInboxSelected

void
GVInbox::loginSuccess ()
{
    QMutexLocker locker(&mutex);
    bLoggedIn = true;
}//GVInbox::loginSuccess

void
GVInbox::loggedOut ()
{
    QMutexLocker locker(&mutex);
    bLoggedIn = false;
}//GVInbox::loggedOut

void
GVInbox::onSigMarkAsRead(const QString &msgId)
{
    AsyncTaskToken *token = new AsyncTaskToken(this);
    token->inParams["id"] = msgId;

    bool rv = connect (token, SIGNAL(completed()),
                       this , SLOT(onInboxEntryMarked()));
    Q_ASSERT(rv); Q_UNUSED(rv);

    MainWindow *mainWin = (MainWindow *) parent ();
    if (!mainWin->gvApi.markInboxEntryAsRead (token)) {
        Q_WARN(QString("Failed to mark read: ID =").arg (msgId));
        delete token;
    }
}//GVInbox::onSigMarkAsRead

void
GVInbox::onInboxEntryMarked ()
{
    AsyncTaskToken *token = (AsyncTaskToken *) QObject::sender ();
    QString id = token->inParams["id"].toString();

    if (ATTS_SUCCESS != token->status) {
        Q_WARN(QString("Failed to mark read: ID =").arg (id));
    } else {
        modelInbox->markAsRead (id);
    }

    token->deleteLater ();
}//GVInbox::onInboxEntryMarked

void
GVInbox::onSigDeleteInboxEntry(const QString &id)
{
    AsyncTaskToken *token = new AsyncTaskToken(this);
    token->inParams["id"] = id;

    bool rv = connect (token, SIGNAL(completed()),
                       this , SLOT(onInboxEntryDeleted()));
    Q_ASSERT(rv); Q_UNUSED(rv);

    MainWindow *mainWin = (MainWindow *) parent ();
    if (!mainWin->gvApi.deleteInboxEntry (token)) {
        Q_WARN(QString("Failed to delete: ID = %1").arg (id));
        delete token;
    }
}//GVInbox::onSigDeleteInboxEntry

void
GVInbox::onInboxEntryDeleted ()
{
    AsyncTaskToken *token = (AsyncTaskToken *) QObject::sender ();

    QString id = token->inParams["id"].toString();

    if (ATTS_SUCCESS != token->status) {
        Q_WARN(QString("Failed to delete: ID = %1").arg (id));
    } else {
        GVInboxEntry hevent;
        hevent.id = id;
        modelInbox->deleteEntry (hevent);
    }

    delete token;
}//GVInbox::onInboxEntryDeleted