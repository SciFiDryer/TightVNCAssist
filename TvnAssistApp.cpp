// Copyright (C) 2009,2010,2011,2012 GlavSoft LLC.
// All rights reserved.
//
//-------------------------------------------------------------------------
// This file is part of the TightVNC software.  Please visit our Web site:
//
//                       http://www.tightvnc.com/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//-------------------------------------------------------------------------
//

#include "TvnAssistApp.h"
#include "tvnserver-app/ServerCommandLine.h"
#include "tvnserver-app/TvnServerHelp.h"

#include "thread/GlobalMutex.h"

#include "util/ResourceLoader.h"
#include "util/StringTable.h"
#include "tvnserver-app/NamingDefs.h"
#include "win-system/WinCommandLineArgs.h"
#include "tvnserver-app/OutgoingRfbConnectionThread.h"


#include "tvnserver/resource.h"


#include "win-system/RegistryKey.h"
#include "tvnserver-app/RfbClientManager.h"

TvnAssistApp::TvnAssistApp(HINSTANCE hInstance,
    const TCHAR* windowClassName,
    const TCHAR* commandLine,
    NewConnectionEvents* newConnectionEvents)
    : WindowsApplication(hInstance, windowClassName),
    m_fileLogger(true),
    m_tvnServer(0),
    m_commandLine(commandLine),
    m_newConnectionEvents(newConnectionEvents)
{
}

TvnAssistApp::~TvnAssistApp()
{
}

int TvnAssistApp::run()
{

    // Start TightVNC server and TightVNC control application.
    try {
        m_tvnServer = new TvnAssistServer(false, m_newConnectionEvents, this, &m_fileLogger);
        m_tvnServer->addListener(this);

    }
    catch (Exception& e) {
        // FIXME: Move string to resource
        StringStorage message;
        message.format(_T("Couldn't run the server: %s"), e.getMessage());
        MessageBox(0,
            message.getString(),
            _T("Server error"), MB_OK | MB_ICONEXCLAMATION);
        return 1;
    }
    try {
        int exitCode = 0;
        if (m_tvnServer->runServer)
        {
            exitCode = WindowsApplication::run();
        }
        MessageBox(0,
            _T("Remote assistance session ended."),
            _T("End of session"), MB_OK | MB_ICONINFORMATION);
        //delete m_tvnControlRunner;
        m_tvnServer->removeListener(this);
        delete m_tvnServer;
        return exitCode;
    }
    catch (Exception& e) {
        // FIXME: Move string to resource
        StringStorage message;
        message.format(_T("Couldn't run the server: %s"), e.getMessage());
        MessageBox(0,
            message.getString(),
            _T("Server error"), MB_OK | MB_ICONEXCLAMATION);
        return 1;
    }
}

void TvnAssistApp::onTvnServerShutdown()
{
    WindowsApplication::shutdown();
}

void TvnAssistApp::onLogInit(const TCHAR* logDir, const TCHAR* fileName,
    unsigned char logLevel)
{
    m_fileLogger.init(logDir, fileName, logLevel);
    m_fileLogger.storeHeader();
}

void TvnAssistApp::onChangeLogProps(const TCHAR* newLogDir, unsigned char newLevel)
{
    m_fileLogger.changeLogProps(newLogDir, newLevel);
}
