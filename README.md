# WinPin

`WinPin` is a Windows utility that automatically saves and restores the position of your windows when you plug in your laptop to an external monitor(s).

Are you tired of having to reposition all your windows every time you come back to your desk after a meeting? Then maybe `WinPin` is for you!

It lives quietly in the system tray and does not require any user interaction. There is no configuration, no settings, no fancy UI, no nothing. Just plug and unplug your monitors as you go and `WinPin` will do the rest.

## Installation

Get the latest release and just run `WinPin.exe`. That's it.

## How it works

`WinPin` periodically (every second) saves the position and state of all your windows for current monitor configuration.

When change in the monitor configuration is detected (e.g. you plug in or unplug a monitor), it waits 3 seconds to let Windows "calm down" and rearrange the desktop and then restores the saved positions.

Adding the waiting period helped fix some issues with monitors connected through USB-C docks.

It does not rely on `WM_DISPLAYCHANGE` or `WM_DEVICECHANGE` messages because that approach turned out to be quite unreliable. Again - especially with some USB-C hubs (I am looking at you, Dell!), which do not properly report monitor changes.

So instead, I am relying on good old polling through `EnumDisplayMonitors` and `GetMonitorInfo` functions. Works like a charm!

## Building

Run 'configure.py' to generate the Visual Studio solution and project files first time in the `.build` directory. Open the solution and get the artifacts from the `.bin` directory.

## Linux or macOS?

Nah...

## License

`Unlicense`

This is free and unencumbered software released into the public domain. I don't care what you do with it. Use it, modify it, sell it, whatever. I don't care. I'm not responsible for anything. Use at your own risk.
