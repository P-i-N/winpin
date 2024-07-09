#pragma once

#include "Base.hpp"

#include <string>
#include <vector>

// Represents a monitor with its position and size.
struct MonitorInfo
{
	std::string name; // Monitor name
	RECT rect;        // Monitor rectangle
	RECT desktopRect; // Monitor rectangle without taskbar
};

enum class WindowState
{
	Normal,
	Minimized,
	Maximized
};

// Represents a window with its position, size and state (minimized, maximized, etc.).
struct WindowInfo
{
	std::string name;                        // Window title
	void *handle = nullptr;                  // HWND
	WINDOWPLACEMENT placement;               // Window position, size, state, etc.
	WindowState state = WindowState::Normal; // Window state (minimized, maximized, etc.)
	int zIndex = 0;                          // Z-index order

	void Restore() const; // Restores the window position
};

// Represents the state of all connected monitors, application windows and their positions
struct State
{
	std::vector<MonitorInfo> monitors; // All connected monitors (sorted by X coordinate from left to right)
	std::vector<WindowInfo> windows;   // All application windows
	size_t monitorsHash = 0;           // Hash of the monitor configuration

	static State Create(); // Creates a new state of the current window & monitor configuration
	void Restore() const;  // Restores all window positions
	void Print() const;    // Prints the state to the console
};
