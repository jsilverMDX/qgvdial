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

#include "Lib.h"

static Lib *singleton = NULL;

Lib &
Lib::ref()
{
    if (NULL == singleton) {
        singleton = new Lib;
    }

    return (*singleton);
}//Lib::ref

void
Lib::deref()
{
    if (NULL != singleton) {
        delete singleton;
        singleton = NULL;
    }
}//Lib::deref

Lib::Lib(QObject *parent)
: QObject(parent)
, m_osd(createOSD (this))
{
}//Lib::Lib