#include "Base.hpp"
#include "Resources.h"
#include "State.hpp"

#include <map>

#define ID_TRAY_APP_ICON 1001
#define WM_TRAYICON ( WM_USER + 1 )

// Queue of old states for each monitor configuration
using StateQueue = std::vector<State>;

// How many old states to keep for each monitor configuration
constexpr size_t NumQueuedStates = 3;

// Map of all known window states on different monitor configurations
std::map<size_t, StateQueue> g_WindowStates;

// Current monitor configuration hash
size_t g_CurrentMonitorsHash = 0;

// Countdown to restore the window positions - we do not restore the window positions immediately after the monitor
// configuration changes, but wait for a few seconds to let everyting stabilize
int g_RestoreCountdown = -1;

// Manually saved state through the context menu
State g_SavedState;

// Global context menu handle
HMENU g_ContextMenu;

//----------------------------------------------------------------------------------------------------------------------
void CALLBACK PeriodicStateCheckTimerFunc( HWND hwnd, UINT message, UINT_PTR idEvent, DWORD dwTime )
{
	auto state = State::Create();
	auto &queue = g_WindowStates[state.monitorsHash];

	LOG( "State hash: {}", state.monitorsHash );

	bool monitorsChanged = ( g_CurrentMonitorsHash != 0 ) && ( state.monitorsHash != g_CurrentMonitorsHash );
	if ( monitorsChanged )
	{
		if ( g_RestoreCountdown < 0 )
			g_RestoreCountdown = 3;

		if ( g_RestoreCountdown > 0 )
		{
			LOG( "Monitor configuration changed, waiting for {} seconds...", g_RestoreCountdown );

			g_RestoreCountdown -= 1;
			return;
		}

		if ( g_RestoreCountdown == 0 )
		{
			LOG( "Restoring window positions..." );

			if ( !queue.empty() )
			{
				state = queue[0];
				state.Restore();
			}
		}
	}

	// Limit the number of old states to keep to `NumQueuedStates`
	queue.erase( queue.begin(), queue.end() - std::min( queue.size(), NumQueuedStates - 1 ) );
	queue.push_back( state );

	g_CurrentMonitorsHash = state.monitorsHash;
	g_RestoreCountdown = -1;
}

//----------------------------------------------------------------------------------------------------------------------
void ShowAboutDialog( HWND hwnd )
{
	auto result = MessageBox( hwnd,
	                          "WinPin 1.0\n\nTool for restoring windows to previous positions,\nwhen monitor layout is "
	                          "changed.\n\nClick 'Help' for GitHub page!",
	                          "About WinPin",
	                          MB_OK | MB_HELP | MB_ICONINFORMATION );
}

//----------------------------------------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	POINT lpClickPoint;

	switch ( message )
	{
		case WM_CREATE: {
			// Check monitor configuration every second and restore window positions if needed
			SetTimer( hwnd, 1, 1000, PeriodicStateCheckTimerFunc );
		}
		break;

		case WM_HELP:
			ShellExecute( nullptr, "open", "https://github.com/P-i-N/winpin", nullptr, nullptr, SW_SHOWNORMAL );
			break;

		case WM_TRAYICON:
			switch ( lParam )
			{
				case WM_LBUTTONUP:
				case WM_RBUTTONUP:
					GetCursorPos( &lpClickPoint ); // Get current mouse position
					SetForegroundWindow( hwnd );   // Set the context menu to be shown

					// TrackPopupMenu blocks the app until TrackPopupMenu returns
					UINT clicked = TrackPopupMenu(
					  g_ContextMenu, TPM_RETURNCMD | TPM_NONOTIFY, lpClickPoint.x, lpClickPoint.y, 0, hwnd, nullptr );

					switch ( clicked )
					{
						case IDI_CMD_SAVE_STATE: {
							g_SavedState = State::Create();
							g_SavedState.Print();
						}
						break;
						case IDI_CMD_RESTORE_STATE: g_SavedState.Restore(); break;
						case IDI_CMD_ABOUT: ShowAboutDialog( hwnd ); break;
						case IDI_CMD_EXIT: PostMessage( hwnd, WM_CLOSE, 0, 0 ); break;
					}

					break;
			}
			break;

		case WM_DESTROY: PostQuitMessage( 0 ); break;

		default: return DefWindowProc( hwnd, message, wParam, lParam );
	}

	return 0;
}

//----------------------------------------------------------------------------------------------------------------------
int APIENTRY WinMain( _In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPSTR lpCmdLine,
                      _In_ int nShowCmd )
{
#ifdef DEBUG
	AllocConsole();
	FILE *pCout;
	freopen_s( &pCout, "CONOUT$", "w", stdout );
#endif

	MSG msg;
	HWND hwnd;
	WNDCLASS wndClass = { 0 };
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = hInstance;
	wndClass.lpszClassName = "WinPinApp";

	RegisterClass( &wndClass );
	hwnd = CreateWindow( "WinPinApp",
	                     "WinPin",
	                     WS_OVERLAPPEDWINDOW,
	                     CW_USEDEFAULT,
	                     0,
	                     CW_USEDEFAULT,
	                     0,
	                     nullptr,
	                     nullptr,
	                     hInstance,
	                     nullptr );

	// Add the icon to the system tray
	NOTIFYICONDATA notifyIconData;
	ZeroMemory( &notifyIconData, sizeof( NOTIFYICONDATA ) );

	notifyIconData.cbSize = sizeof( NOTIFYICONDATA );
	notifyIconData.hWnd = hwnd;
	notifyIconData.uID = ID_TRAY_APP_ICON;
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIconData.uCallbackMessage = WM_TRAYICON;
	notifyIconData.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_APP_ICON ) );
	strcpy( notifyIconData.szTip, "WinPin" );

	Shell_NotifyIcon( NIM_ADD, &notifyIconData );

	// Load the context menu
	g_ContextMenu = CreatePopupMenu();

	// Populate the context menu
	AppendMenu( g_ContextMenu, MF_STRING, IDI_CMD_SAVE_STATE, TEXT( "Save state" ) );
	AppendMenu( g_ContextMenu, MF_STRING, IDI_CMD_RESTORE_STATE, TEXT( "Restore state" ) );
	AppendMenu( g_ContextMenu, MF_SEPARATOR, 0, TEXT( "-" ) );
	AppendMenu( g_ContextMenu, MF_STRING, IDI_CMD_ABOUT, TEXT( "About..." ) );
	AppendMenu( g_ContextMenu, MF_SEPARATOR, 0, TEXT( "-" ) );
	AppendMenu( g_ContextMenu, MF_STRING, IDI_CMD_EXIT, TEXT( "Exit" ) );

	// Main message loop
	while ( GetMessage( &msg, nullptr, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	// Remove the icon from the system tray
	Shell_NotifyIcon( NIM_DELETE, &notifyIconData );

	return ( int )msg.wParam;
}
