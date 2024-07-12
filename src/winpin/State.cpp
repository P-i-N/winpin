#include "State.hpp"

#include <Windows.h>

#include <algorithm>

//----------------------------------------------------------------------------------------------------------------------
void WindowInfo::Restore() const
{
	auto hwnd = ( HWND )handle;

	if ( !IsWindow( hwnd ) )
		hwnd = FindWindow( nullptr, name.c_str() );

	if ( hwnd )
	{
		auto currentWindowState = IsIconic( hwnd )   ? WindowState::Minimized
		                          : IsZoomed( hwnd ) ? WindowState::Maximized
		                                             : WindowState::Normal;

		if ( currentWindowState != WindowState::Normal )
			ShowWindow( hwnd, SW_RESTORE );

		auto p = placement; // Make a mutable copy
		switch ( state )
		{
			case WindowState::Normal: p.showCmd = SW_SHOWNORMAL; break;
			case WindowState::Minimized: p.showCmd = SW_SHOWMINIMIZED; break;
			case WindowState::Maximized: p.showCmd = SW_SHOWMAXIMIZED; break;
		};

		// SetWindowPlacement is a bit flaky, when you have different scalings on different monitors.
		// Calling it only once caused some windows to be resized incorrectly.
		// Calling it twice seems to fix the issue. I don't know why.
		SetWindowPlacement( hwnd, &p );
		SetWindowPlacement( hwnd, &p );

		if ( state != WindowState::Minimized )
			SetForegroundWindow( hwnd );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------------------------------------------
BOOL CALLBACK MonitorEnumProc( HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData )
{
	auto *state = ( State * )dwData;

	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof( MONITORINFOEX );
	GetMonitorInfo( hMonitor, &monitorInfo );

	MonitorInfo monitor;
	monitor.rect = *lprcMonitor;
	monitor.name = monitorInfo.szDevice;
	monitor.desktopRect = monitorInfo.rcWork;

	state->monitors.push_back( monitor );

	return TRUE;
}

//----------------------------------------------------------------------------------------------------------------------
BOOL CALLBACK WindowEnumProc( HWND hwnd, LPARAM lParam )
{
	auto *state = ( State * )lParam;

	if ( IsWindowVisible( hwnd ) )
	{
		char title[256] = { 0 };
		GetWindowText( hwnd, title, sizeof( title ) );

		// Skip windows with empty titles, they are usually not important
		if ( title[0] == 0 )
			return TRUE;

		WindowInfo window;
		window.name = title;
		window.handle = hwnd;

		ZeroMemory( &window.placement, sizeof( WINDOWPLACEMENT ) );
		window.placement.length = sizeof( WINDOWPLACEMENT );
		GetWindowPlacement( hwnd, &window.placement );

		window.state = IsIconic( hwnd )   ? WindowState::Minimized
		               : IsZoomed( hwnd ) ? WindowState::Maximized
		                                  : WindowState::Normal;

		// Determine z-index of the window
		HWND hwndNext = GetWindow( hwnd, GW_HWNDNEXT );
		while ( hwndNext )
		{
			window.zIndex += 1;
			hwndNext = GetWindow( hwndNext, GW_HWNDNEXT );
		}

		// Only windows with z-index > 1 are considered. "Program Manager"/desktop has z-index 1 and everything else
		// MUST be above it
		if ( window.zIndex > 1 )
			state->windows.push_back( window );
	}

	return TRUE;
}

//----------------------------------------------------------------------------------------------------------------------
State State::Create()
{
	State state;

	// Enumerate and sort windows
	{
		EnumWindows( WindowEnumProc, ( LPARAM )&state );

		// Sort windows by z-index. They should be already ordered by EnumWindows, but just in case...
		std::sort( state.windows.begin(), state.windows.end(), []( const WindowInfo &a, const WindowInfo &b ) {
			return a.zIndex < b.zIndex;
		} );
	}

	// Enumerate and sort monitors
	{
		EnumDisplayMonitors( nullptr, nullptr, MonitorEnumProc, ( LPARAM )&state );

		// Sort monitors by X coordinate from left to right. If X is the same, sort by Y coordinate from top to bottom.
		std::sort( state.monitors.begin(), state.monitors.end(), []( const MonitorInfo &a, const MonitorInfo &b ) {
			if ( a.rect.left == b.rect.left )
				return a.rect.top < b.rect.top;

			return a.rect.left < b.rect.left;
		} );
	}

	// Hash monitors by topLeft and bottomRight coordinates
	for ( const auto &monitor : state.monitors )
	{
		uint64_t topLeft = monitor.rect.left | ( ( uint64_t )monitor.rect.top << 32 );
		uint64_t bottomRight = monitor.rect.right | ( ( uint64_t )monitor.rect.bottom << 32 );

		state.monitorsHash ^= std::hash<uint64_t>()( topLeft );
		state.monitorsHash ^= std::hash<uint64_t>()( bottomRight );
	}

	return state;
}

//----------------------------------------------------------------------------------------------------------------------
void State::Restore() const
{
	for ( auto &window : windows )
		window.Restore();
}

//----------------------------------------------------------------------------------------------------------------------
void State::Print() const
{
	printf( "Monitors:\n" );
	for ( const auto &monitor : monitors )
	{
		printf( "  %s: %d,%d %d,%d\n",
		        monitor.name.c_str(),
		        monitor.rect.left,
		        monitor.rect.top,
		        monitor.rect.right,
		        monitor.rect.bottom );
	}

	printf( "Windows:\n" );
	for ( const auto &window : windows )
	{
		printf( "  %p: %s\n", window.handle, window.name.c_str() );
		printf( "      rect = [%d; %d]-[%d; %d]\n",
		        window.placement.rcNormalPosition.left,
		        window.placement.rcNormalPosition.top,
		        window.placement.rcNormalPosition.right,
		        window.placement.rcNormalPosition.bottom );
	}
}
