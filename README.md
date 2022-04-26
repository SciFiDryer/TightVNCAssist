# TightVNCAssist

TightVNC powered remote assistance for Windows. This project is not affiliated with TightVNC.

## Overview

This project is a slightly modified version of TightVNC that:

- Opens as an application
- Requires no external files or DLLs
- Immediately prompts user for a remote VNC viewer to connect to (listening on port 5500)
- Does not listen for local VNC connections
- Does not launch System tray icon
- Terminates after remote viewer disconnects and informs user that the session has ended.


## To use

Download the TightVNC.exe file and launch. Type in an IP of a remote listening VNC viewer. The executable is portable and should be able to run unprivileged.
