#include "ConnectionDialog.h"
#include "Resource.h"
#include <windows.h>
#include <iostream>

TCHAR hostAddr[50];

BOOL CALLBACK ConnectCallback(HWND hwndDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    
    switch (message)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            GetDlgItemText(hwndDlg,
                IDC_EDIT1,
                (LPWSTR)&hostAddr,
                50);
            EndDialog(hwndDlg, wParam);
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            *hostAddr = NULL;
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}
void ConnectionDialog::showModel()
{
	DialogBox(NULL,
		MAKEINTRESOURCE(IDD_DIALOG1),
		NULL,
		(DLGPROC)ConnectCallback);
}
TCHAR* ConnectionDialog::getHostAddr()
{
    return hostAddr;
}

