# TODO Tasks for AltTab

## Features
- [x] Implement basic Alt+Tab functionality.
	- [x] Tab, Shift+Tab
	- [x] Backtick, Shift+Backtick
	- [x] Escape
- [x] Add support for custom key bindings.
	- [x] Delete        : Terminate window/process
	- [x] Backtick      : To switch to similar process
	- [x] Shift+Backtick: To switch to similar process
	- [x] Shift+Delete  : Kill Window
- [x] System tray.
	- [x] Add application to system tray.
	- [x] Group 1: About
	- [x] Separator ---------------------------
	- [x] Group 2: ReadMe
	- [x] Group 2: Help
	- [x] Group 2: Release Notes
	- [x] Separator ---------------------------
	- [x] Group 3: Settings
	- [x] Group 3: Disable AltTab
	- [x] Group 3: Check for updates
	- [x] Group 3: Run At Startup
	- [x] Separator ---------------------------
	- [x] Group 4: Restart
	- [x] Group 4: Exit
- [x] Add search string functionality
- [x] Settings & Settings UI
	- [x] Load & Save settings from & to AltTabSettings.ini file
	- [x] Prompt terminate all
	- [x] Fuzzy string match
	- [x] Transparency
	- [x] Window width & height
	- [x] Check for updates
	- [x] Exclude processes
- [x] Custom icon
- [x] Add Context Menu
	- [x] Group 1:  | Close          | Delete       |
	- [x] Group 1:  | Terminate      | Shift+Delete |
	- [x] Separator |----------------+--------------|
	- [x] Group 2:  | &Close All     |              |
	- [x] Group 2:  | &Terminate All |              |
	- [x] Separator |----------------+--------------|
	- [x] Group 3:  | &Open Path     |              |
	- [x] Group 3:  | Copy &Path     |              |
	- [x] Separator |----------------+--------------|
	- [x] Group 4:  | About          | Shift+F1	    |
	- [x] Group 4:  | Settings       | F2           |
- [ ] Logger
	- [x] Customize logging
	- [x] Rollover beyoung the max size
	- [x] Log to file
	- [x] Log to console
	- [ ] Use external .properties file for log4cpp
- [ ] Custom tooltip


## Bug Fixes
- [x] Resolve issue with window focus not updating correctly.

## Known issues
- [ ] AltTab hot keys are not working on elevated processs if AltTab is running as a non-elevated process.

## Refactoring
- [ ] Clean up and organize source code.
- [ ] Improve code documentation.

## Documentation
- [ ] Update README with project description and usage instructions.
- [ ] Document architecture and code structure.
