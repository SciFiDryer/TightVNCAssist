// TightVNCAssist.cpp : Defines the entry point for the application.
//

//#include "framework.h"
//#include "TightVNCAssist.h"
#include "util/CommonHeader.h"
#include "util/winhdr.h"
#include "util/CommandLine.h"
#include "win-system/WinCommandLineArgs.h"

#include "tvnserver-app/TvnService.h"
#include "TvnAssistApp.h"
#include "tvnserver-app/QueryConnectionApplication.h"
#include "tvnserver-app/DesktopServerApplication.h"
#include "tvnserver-app/AdditionalActionApplication.h"
#include "tvnserver-app/ServiceControlApplication.h"
#include "tvnserver-app/ServiceControlCommandLine.h"
#include "tvnserver-app/QueryConnectionCommandLine.h"
#include "tvnserver-app/DesktopServerCommandLine.h"

#include "tvncontrol-app/ControlApplication.h"
#include "tvncontrol-app/ControlCommandLine.h"

#include "tvnserver/resource.h"
#include "tvnserver-app/CrashHook.h"
#include "tvnserver-app/NamingDefs.h"

#include "tvnserver-app/WinEventLogWriter.h"


int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow)
{
    LogWriter preLog(0);

    // Life time of the sysLog must be greater than a TvnService object
    // because the crashHook uses it but fully functional usage possible
    // only after the TvnService object start.
    WinEventLogWriter winEventLogWriter(&preLog);
    CrashHook crashHook(&winEventLogWriter);

    ResourceLoader resourceLoaderSingleton(hInstance);

    // No additional applications, run TightVNC server as single application.
    crashHook.setGuiEnabled();
    TvnAssistApp tvnServer(hInstance,
        WindowNames::WINDOW_CLASS_NAME,
        lpCmdLine, &winEventLogWriter);

    return tvnServer.run();

}
