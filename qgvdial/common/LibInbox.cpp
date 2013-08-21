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

#include "LibInbox.h"
#include "IMainWindow.h"
#include "InboxModel.h"

LibInbox::LibInbox(IMainWindow *parent)
: QObject(parent)
{
    connect (&parent->gvApi,
             SIGNAL(oneInboxEntry(AsyncTaskToken*,GVInboxEntry)),
             this,
             SLOT(onOneInboxEntry(AsyncTaskToken*,GVInboxEntry)));
}//LibInbox::LibInbox

InboxModel *
LibInbox::createModel()
{
    IMainWindow *win = (IMainWindow *) this->parent ();
    InboxModel *modelInbox = new InboxModel(this);

    win->db.refreshInboxModel (modelInbox, "all");

    return (modelInbox);
}//LibInbox::createModel

bool
LibInbox::refresh(const char *type, QDateTime after)
{
    AsyncTaskToken *task = new AsyncTaskToken(this);
    if (NULL == task) {
        Q_WARN("Failed to allocate task");
        return false;
    }

    return beginRefresh (task, type, after, 1);
}//LibInbox::refresh

bool
LibInbox::beginRefresh(AsyncTaskToken *task, QString type, QDateTime after,
                       int page)
{
    task->reinit ();

    task->inParams["type"] = type;
    task->inParams["page"] = page;
    task->inParams["after"] = after;

    connect (task, SIGNAL(completed()), this, SLOT(onRefreshDone()));

    IMainWindow *win = (IMainWindow *) this->parent ();
    bool rv = win->gvApi.getInbox (task);

    win->db.setQuickAndDirty (true);

    if (!rv) {
        task->status = ATTS_FAILURE;
        task->emitCompleted ();
        rv = true;
    }

    return (rv);
}//LibInbox::beginRefresh

void
LibInbox::onOneInboxEntry (AsyncTaskToken *task, const GVInboxEntry &hevent)
{
    QDateTime after = task->inParams["after"].toDateTime();

    if ((!after.isValid ()) || (hevent.startTime >= after)) {
        // Stuff this into the DB only if it is after "after"
        IMainWindow *win = (IMainWindow *) this->parent ();
        win->db.insertInboxEntry (hevent);
    }

    if (after.isValid () && (hevent.startTime >= after)) {
        bool overflow = true;
        task->inParams["overflow"] = overflow;
    }
}//LibInbox::onOneInboxEntry

void
LibInbox::onRefreshDone()
{
    IMainWindow *win = (IMainWindow *) this->parent ();
    AsyncTaskToken *task = (AsyncTaskToken *) QObject::sender ();

    win->db.setQuickAndDirty (false);

    do {
        if (ATTS_SUCCESS != task->status) {
            Q_WARN(QString("Failed to get inbox page")
                   .arg(task->inParams["page"].toInt()));
        }

        int page = task->inParams["page"].toInt();
        bool overflow = task->inParams["overflow"].toBool();
        if ((page > 30) || overflow) {
            task->status = ATTS_SUCCESS;
            break;
        }

        QString type = task->inParams["type"].toString();
        QDateTime after = task->inParams["after"].toDateTime();

        page++;

        if (beginRefresh (task, type, after, page)) {
            task = NULL;
        }
    } while (0);

    if (task) {
        emit sigRefreshed (ATTS_SUCCESS == task->status);
        task->deleteLater ();
    }
}//LibInbox::onRefreshDone