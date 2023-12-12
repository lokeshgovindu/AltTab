# TODO Tasks for AltTab

## Features
- [x] Implement basic Alt+Tab functionality.
	- [x] Tab, Shift+Tab
	- [x] Backtick, Shift+Backtick
	- [x] Escape
- [ ] Add support for custom key bindings.
	- [x] Delete        : Terminate window/process
	- [x] Backtick      : To switch to similar process
	- [x] Shift+Backtick: To switch to similar process
	- [x] Shift+Delete  : Kill Window
- [ ] System tray.
	- [x] Add application to system tray.
	- [x] Group 1: About
	- [ ] Group 2: ReadMe
	- [ ] Group 2: Help
	- [ ] Group 2: Release Notes
	- [x] Group 3: Settings
	- [x] Group 3: Disable AltTab
	- [ ] Group 3: Check for updates
	- [x] Group 3: Run At Startup
	- [x] Group 3: Restart
	- [x] Group 4: Exit
- [ ] Add search string functionality
- [x] Settings & Settings UI
	- [x] Load & Save settings from & to AltTabSettings.ini file
	- [ ] Prompt terminate all
	- [ ] Fuzzy string match
	- [x] Transparency
	- [x] Window width & height
	- [ ] Check for updates
	- [ ] Exclude processes
- [ ] Custom icon
- [ ] Add Context Menu
	- [ ] Group 1: Terminate Window  : Delete
	- [ ] Group 1: Kill Process      : Shift+Delete
	- [ ] Group 2: Close All Windows : NumpadDiv(/)
	- [ ] Group 3: About             : Shift+F1
	- [ ] Group 3: ReadMe
	- [ ] Group 3: Help              : F1
	- [ ] Group 3: Release Notes
	- [ ] Group 3: Settings          : F2
	- [ ] Group 4: Exit

## Bug Fixes
- [ ] Resolve issue with window focus not updating correctly.

## Known issues
- [ ] AltTab hot keys are not working on elevated processs if AltTab is running as a non-elevated process.

## Refactoring
- [ ] Clean up and organize source code.
- [ ] Improve code documentation.

## Documentation
- [ ] Update README with project description and usage instructions.
- [ ] Document architecture and code structure.
