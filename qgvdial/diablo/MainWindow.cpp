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
#include "MainWindow_p.h"
#include "ui_mainwindow.h"
#include "ContactsModel.h"
#include "InboxModel.h"

QCoreApplication *
createApplication(int argc, char *argv[])
{
    return new QApplication(argc, argv);
}

MainWindow::MainWindow(QObject *parent)
: IMainWindow(parent)
, d(new MainWindowPrivate)
, contactsModel(NULL)
, inboxModel(NULL)
{
}//MainWindow::MainWindow

MainWindow::~MainWindow()
{
    delete d;
}//MainWindow::~MainWindow

void
MainWindow::init()
{
    IMainWindow::init ();

    // Desktop only ?
    Qt::WindowFlags flags = d->windowFlags ();
    flags &= ~(Qt::WindowMaximizeButtonHint);
    d->setWindowFlags (flags);
    d->setFixedSize (d->size ());

    d->setOrientation(MainWindowPrivate::ScreenOrientationAuto);
    d->showExpanded();
    connect(d->ui->loginButton, SIGNAL(clicked()),
            this, SLOT(onLoginClicked()));

    QTimer::singleShot (1, this, SLOT(onInitDone()));
}//MainWindow::init

void
MainWindow::log(QDateTime dt, int level, const QString &strLog)
{
}//MainWindow::log

void
MainWindow::uiRequestLoginDetails()
{
    QMessageBox msg;
    msg.setText ("Please enter a user and password");
    msg.exec ();

    d->ui->tabWidget->setCurrentIndex (3);
    d->ui->toolBox->setCurrentIndex (0);
}//MainWindow::uiRequestLoginDetails

void
MainWindow::onLoginClicked()
{
    if ("Login" == d->ui->loginButton->text()) {
        QString user, pass;
        user = d->ui->textUsername->text ();
        pass = d->ui->textPassword->text ();

        if (user.isEmpty () || pass.isEmpty ()) {
            Q_WARN("Invalid username or password");
            QMessageBox msg;
            msg.setText (tr("Invalid username or password"));
            msg.exec ();
            return;
        }

        beginLogin (user, pass);
    } else {
        onUserLogoutRequest ();
    }
}//MainWindow::onLoginClicked

void
MainWindow::onUserLogoutDone()
{
    Q_DEBUG("Logout complete");
}//MainWindow::onUserLogoutDone

void
MainWindow::uiRequestTFALoginDetails(void *ctx)
{
    QString strPin = QInputDialog::getText(d, tr("Enter PIN"),
                                           tr("Two factor authentication"));

    int pin = strPin.toInt ();
    if (pin == 0) {
        resumeTFAAuth (ctx, pin, true);
    } else {
        resumeTFAAuth (ctx, pin, false);
    }
}//MainWindow::uiRequestTFALoginDetails

void
MainWindow::uiSetUserPass(bool editable)
{
    d->ui->textUsername->setText (m_user);
    d->ui->textPassword->setText (m_pass);

    d->ui->textUsername->setReadOnly (!editable);
    d->ui->textPassword->setReadOnly (!editable);
    d->ui->textUsername->setEnabled (editable);
    d->ui->textPassword->setEnabled (editable);

    d->ui->loginButton->setText (editable ? "Login" : "Logout");
}//MainWindow::uiSetUserPass

void
MainWindow::uiRequestApplicationPassword()
{
    bool ok;
    QString strAppPw =
    QInputDialog::getText (d, "Application specific password",
                           "Enter password for contacts", QLineEdit::Password,
                           "", &ok);
    if (ok) {
        onUiGotApplicationPassword(strAppPw);
    }
}//MainWindow::uiRequestApplicationPassword

void
MainWindow::uiLoginDone(int status, const QString &errStr)
{
    do {
        if (ATTS_NW_ERROR == status) {
            d->ui->statusBar->showMessage ("Network error. Try again later.");
            break;
        } else if (ATTS_USER_CANCEL == status) {
            d->ui->statusBar->showMessage ("User canceled login.");
            break;
        } else if (ATTS_SUCCESS != status) {
            QMessageBox msg;
            msg.setIcon (QMessageBox::Critical);
            msg.setText (errStr);
            msg.setWindowTitle ("Login failed");
            msg.exec ();
            break;
        }

        d->ui->statusBar->showMessage ("Login successful");
    } while (0);
}//MainWindow::uiLoginDone

void
MainWindow::uiRefreshContacts()
{
    contactsModel = oContacts.createModel ();
    QAbstractItemModel *oldModel = d->ui->contactsView->model ();
    d->ui->contactsView->setModel (contactsModel);
    if (NULL != oldModel) {
        delete oldModel;
    }

    d->ui->contactsView->hideColumn (0);
    d->ui->contactsView->hideColumn (3);
    d->ui->contactsView->hideColumn (4);
}//MainWindow::uiRefreshContacts

void
MainWindow::uiRefreshInbox()
{
    inboxModel = oInbox.createModel ();
    QAbstractItemModel *oldModel = d->ui->inboxView->model ();
    d->ui->inboxView->setModel (inboxModel);
    if (NULL != oldModel) {
        delete oldModel;
    }

    d->ui->inboxView->hideColumn (0);
    d->ui->inboxView->hideColumn (4);
    d->ui->inboxView->hideColumn (5);
    d->ui->inboxView->hideColumn (7);
}//MainWindow::uiRefreshInbox