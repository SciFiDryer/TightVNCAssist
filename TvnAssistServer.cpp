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

#include "TvnAssistServer.h"
#include "tvnserver-app/WsConfigRunner.h"
#include "tvnserver-app/AdditionalActionApplication.h"
#include "win-system/CurrentConsoleProcess.h"
#include "win-system/Environment.h"

#include "server-config-lib/Configurator.h"

#include "thread/GlobalMutex.h"

#include "tvnserver/resource.h"

#include "wsconfig-lib/TvnLogFilename.h"
#include "resource.h"
#include "network/socket/WindowsSocket.h"

#include "util/StringTable.h"
#include "util/AnsiStringStorage.h"
#include "tvnserver-app/NamingDefs.h"

#include "file-lib/File.h"

// FIXME: Bad dependency on tvncontrol-app.
#include "tvncontrol-app/TransportFactory.h"
#include "tvncontrol-app/ControlPipeName.h"

#include "tvnserver/BuildTime.h"
#include "ConnectionDialog.h"
#include <crtdbg.h>
#include <time.h>
#include "TimeAPI.h"
#include <tvnserver-app/ControlClient.h>
#include "tvncontrol-app/OutgoingConnectionDialog.h"
#include <tvncontrol-app/MakeRfbConnectionCommand.h>
#include <tvncontrol-app/ControlCommand.h>


TvnAssistServer::TvnAssistServer(bool runsInServiceContext,
    NewConnectionEvents* newConnectionEvents,
    LogInitListener* logInitListener,
    Logger* logger)
    : Singleton<TvnAssistServer>(),
    ListenerContainer<TvnServerListener*>(),
    m_runAsService(runsInServiceContext),
    m_logInitListener(logInitListener),
    m_rfbClientManager(0),
    m_httpServer(0), m_controlServer(0), m_rfbServer(0),
    m_config(runsInServiceContext),
    m_log(logger),
    m_contextSwitchResolution(1),
    m_extraRfbServers(&m_log)
{
    m_log.message(_T("%s Build on %s"),
        ProductNames::SERVER_PRODUCT_NAME,
        BuildTime::DATE);

    // Initialize windows sockets.

    m_log.info(_T("Initialize WinSock"));

    try {
        WindowsSocket::startup(2, 1);
    }
    catch (Exception& ex) {
        m_log.interror(_T("%s"), ex.getMessage());
    }

    DesktopFactory* desktopFactory = 0;
    if (runsInServiceContext) {
        desktopFactory = &m_serviceDesktopFactory;
    }
    else {
        desktopFactory = &m_applicationDesktopFactory;
    }

    m_rfbClientManager = new RfbClientManager(0, newConnectionEvents, &m_log, desktopFactory);

    m_rfbClientManager->addListener(this);
    
    runServer = false;
    ConnectionDialog connectDlg = ConnectionDialog();
    connectDlg.showModel();
    TCHAR* hostAddr = connectDlg.getHostAddr();
    if (hostAddr != NULL && _tcscmp(hostAddr, _T("")) != 0)
    {
        SocketIPv4* socket = new SocketIPv4();
        try 
        {
            socket->connect(hostAddr, 5500);
            runServer = true;
        }
        catch (Exception& someEx)
        {
            //m_log.error(_T("Failed to connect to %s:%d with reason: '%s'"),
            //    connDialog.getConnectString(), 5500, someEx.getMessage());
            MessageBox(0,
                someEx.getMessage(),
                _T("Connection error"), MB_OK | MB_ICONEXCLAMATION);
            generateExternalShutdownSignal();
            delete socket;
            return;
        }

        m_rfbClientManager->addNewConnection(socket,
            &ViewPortState(), // with a default view port
            false, true);
    }


}

void TvnAssistServer::onConfigReload(ServerConfig* serverConfig)
{
        // FIXME: Protect only primitive operations.
        // FIXME: Nested lock in protected code (congifuration locking).
        AutoLock l(&m_mutex);

        bool toggleMainRfbServer =
            m_srvConfig->isAcceptingRfbConnections() != (m_rfbServer != 0);
        bool changeMainRfbPort = m_rfbServer != 0 &&
            (m_srvConfig->getRfbPort() != (int)m_rfbServer->getBindPort());

        const TCHAR* bindHost =
            m_srvConfig->isOnlyLoopbackConnectionsAllowed() ? _T("localhost") : _T("0.0.0.0");
        bool changeBindHost = m_rfbServer != 0 &&
            _tcscmp(m_rfbServer->getBindHost(), bindHost) != 0;

        if (toggleMainRfbServer ||
            changeMainRfbPort ||
            changeBindHost) {
            //restartMainRfbServer();
        }

        // NOTE: ExtraRfbServers::reload() does not throw exceptions if some
        //       servers did not start. However, it returns false in that case.
        //       Here we ignore all errors.
        (void)m_extraRfbServers.reload(m_runAsService, m_rfbClientManager);
}
TvnAssistServer::~TvnAssistServer()
{

    ZombieKiller* zombieKiller = ZombieKiller::getInstance();

    // Disconnect all zombies http, rfb, control clients though killing
    // their threads.
    zombieKiller->killAllZombies();

    m_rfbClientManager->removeListener(this);

    delete m_rfbClientManager;

    m_log.info(_T("Shutdown WinSock"));

    try {
        WindowsSocket::cleanup();
    }
    catch (Exception& ex) {
        m_log.error(_T("%s"), ex.getMessage());
    }
}

void TvnAssistServer::generateExternalShutdownSignal()
{
    AutoLock l(&m_listeners);

    vector<TvnServerListener*>::iterator it;
    for (it = m_listeners.begin(); it != m_listeners.end(); it++) {
        TvnServerListener* each = *it;

        each->onTvnServerShutdown();
    } // for all listeners.
}

void TvnAssistServer::afterFirstClientConnect()
{
    if (timeBeginPeriod(m_contextSwitchResolution) == TIMERR_NOERROR) {
        m_log.message(_T("Set context switch resolution: %d ms"), m_contextSwitchResolution);
    }
    else {
        m_log.message(_T("Can't change context switch resolution to: %d ms"), m_contextSwitchResolution);
    }

}

void TvnAssistServer::afterLastClientDisconnect()
{
    m_log.message(_T("Restore context switch resolution"));
    timeEndPeriod(m_contextSwitchResolution);
    generateExternalShutdownSignal();
}
