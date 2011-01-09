#include "GVH_XMLJsonHandler.h"
#include <QtScript>
#include <QtXmlPatterns>

GVH_XMLJsonHandler::GVH_XMLJsonHandler (QObject *parent)
: QObject (parent)
, nUsableMsgs (0)
{
}//GVH_XMLJsonHandler::GVH_XMLJsonHandler

bool
GVH_XMLJsonHandler::startElement (const QString        & /*namespaceURI*/,
                                  const QString        & /*localName   */,
                                  const QString        & /*qName       */,
                                  const QXmlAttributes & /*atts        */)
{
    strChars.clear ();
    return (true);
}//GVH_XMLJsonHandler::startElement

bool
GVH_XMLJsonHandler::endElement (const QString & /*namespaceURI*/,
                               const QString &localName   ,
                               const QString & /*qName       */)
{
    if (localName == "json") {
        strJson = strChars;
        qDebug ("Got json characters");
    }
    if (localName == "html") {
        strHtml = "<html>"
                + strChars
                + "</html>";
        qDebug ("Got html characters");

        QXmlInputSource inputSource;
        inputSource.setData (strHtml);
        QXmlSimpleReader simpleReader;

        simpleReader.setContentHandler (&smsHandler);
        simpleReader.setErrorHandler (&smsHandler);
        simpleReader.parse (&inputSource, false);
    }
    return (true);
}//GVH_XMLJsonHandler::endElement

bool
GVH_XMLJsonHandler::characters (const QString &ch)
{
    strChars += ch;
    return (true);
}//GVH_XMLJsonHandler::characters

bool
GVH_XMLJsonHandler::parseJSON (const QDateTime &dtUpdate, bool &bGotOld)
{
    bool rv = false;
    do // Begin cleanup block (not a loop)
    {
        QString strTemp;
        QScriptEngine scriptEngine;
        strTemp = "var topObj = " + strJson;
        scriptEngine.evaluate (strTemp);

        strTemp = "var msgParams = []; "
                  "var msgList = []; "
                  "for (var msgId in topObj[\"messages\"]) { "
                  "    msgList.push(msgId); "
                  "}";
        scriptEngine.evaluate (strTemp);
        if (scriptEngine.hasUncaughtException ()) {
            strTemp = QString ("Uncaught exception executing script : %1")
                      .arg (scriptEngine.uncaughtException ().toString ());
            qWarning () << strTemp;
            break;
        }

        qint32 nMsgCount = scriptEngine.evaluate("msgList.length;").toInt32 ();
        qDebug () << QString ("message count = %1").arg (nMsgCount);

        qint32 nOldMsgs = 0;

        for (qint32 i = 0; i < nMsgCount; i++) {
            strTemp = QString(
                    "msgParams = []; "
                    "for (var params in topObj[\"messages\"][msgList[%1]]) { "
                    "    msgParams.push(params); "
                    "}").arg(i);
            scriptEngine.evaluate (strTemp);
            if (scriptEngine.hasUncaughtException ()) {
                strTemp = QString ("Uncaught exception in message loop: %1")
                          .arg (scriptEngine.uncaughtException ().toString ());
                qWarning () << strTemp;
                break;
            }

            qint32 nParams =
            scriptEngine.evaluate ("msgParams.length;").toInt32 ();

            GVInboxEntry inboxEntry;
            for (qint32 j = 0; j < nParams; j++) {
                strTemp = QString("msgParams[%1];").arg (j);
                QString strPName = scriptEngine.evaluate (strTemp).toString ();
                strTemp = QString(
                          "topObj[\"messages\"][msgList[%1]][msgParams[%2]];")
                            .arg (i)
                            .arg (j);
                QString strVal = scriptEngine.evaluate (strTemp).toString ();

                if (strPName == "id") {
                    inboxEntry.id = strVal;
                } else if (strPName == "phoneNumber") {
                    inboxEntry.strPhoneNumber = strVal;
                } else if (strPName == "displayNumber") {
                    inboxEntry.strDisplayNumber = strVal;
                } else if (strPName == "startTime") {
                    bool bOk = false;
                    quint64 iVal = strVal.toULongLong (&bOk) / 1000;
                    if (bOk) {
                        inboxEntry.startTime = QDateTime::fromTime_t (iVal);
                    }
                } else if (strPName == "isRead") {
                    inboxEntry.bRead = (strVal == "true");
                } else if (strPName == "isSpam") {
                    inboxEntry.bSpam = (strVal == "true");
                } else if (strPName == "isTrash") {
                    inboxEntry.bTrash = (strVal == "true");
                } else if (strPName == "star") {
                    inboxEntry.bStar = (strVal == "true");
                } else if (strPName == "labels") {
                    if (strVal.contains ("placed")) {
                        inboxEntry.Type = GVIE_Placed;
                    } else if (strVal.contains ("received")) {
                        inboxEntry.Type = GVIE_Received;
                    } else if (strVal.contains ("missed")) {
                        inboxEntry.Type = GVIE_Missed;
                    } else if (strVal.contains ("voicemail")) {
                        inboxEntry.Type = GVIE_Voicemail;
                    } else if (strVal.contains ("sms")) {
                        inboxEntry.Type = GVIE_TextMessage;
                    } else {
                        qWarning () << QString("Unknown label %1").arg(strVal);
                    }
                } else if (strPName == "displayStartDateTime") {
                } else if (strPName == "displayStartTime") {
                } else if (strPName == "relativeStartTime") {
                } else if (strPName == "note") {
                } else if (strPName == "type") {
                } else if (strPName == "children") {
                } else {
                    qDebug () << QString ("param = %1. value = %2")
                                    .arg (strPName)
                                    .arg (strVal);
                }
            }

            if (0 == inboxEntry.id.size()) {
                qWarning ("Invalid ID");
                continue;
            }
            if (0 == inboxEntry.strPhoneNumber.size()) {
                qWarning ("Invalid Phone number");
                continue;
            }
            if (0 == inboxEntry.strDisplayNumber.size()) {
                inboxEntry.strDisplayNumber = "Unknown";
            }
            if (!inboxEntry.startTime.isValid ()) {
                qWarning ("Invalid start time");
                continue;
            }

            // Check to see if it is too old to show
            if (dtUpdate.isValid () && (dtUpdate >= inboxEntry.startTime))
            {
                nOldMsgs++;
                if (1 == nOldMsgs) {
                    qDebug ("Started getting old entries.");
                    bGotOld = true;
                } else {
                    qDebug ("Another old entry");
                }
            }

            // Pick up the text from the parsed HTML
            if ((GVIE_TextMessage == inboxEntry.Type) &&
                (smsHandler.mapTexts.contains (inboxEntry.id)))
            {
                inboxEntry.strText = smsHandler.mapTexts[inboxEntry.id];
            }

            // emit the history element
            emit oneElement (inboxEntry);
            nUsableMsgs++;
        }

        qDebug () << QString ("Valid messages = %1").arg (nUsableMsgs);
        qDebug () << QString ("Old messages = %1").arg (nOldMsgs);

        rv = true;
    } while (0); // End cleanup block (not a loop)

    return (rv);
}//GVH_XMLJsonHandler::parseJSON

qint32
GVH_XMLJsonHandler::getUsableMsgsCount ()
{
    return (nUsableMsgs);
}//GVH_XMLJsonHandler::getUsableMsgsCount
