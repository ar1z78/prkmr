
MishRoller 1.0.0 by Ar1z for Project Rubi-Ka
=====================================================
https://github.com/ar1z78/prkmr


Description
-----------

MishRoller is a specialized mission-rolling and packet-parsing companion utility designed for Project Rubi-Ka. It aggregates raw data from game terminal packages to showcase details for all 5 pulled missions inside a graphical user interface.  Additionally, it can search for substrings in item names and can warn you when a mission's reward match against one of the custom criteria. THose can be reward items, target playfields, precise coordinates, mission type classes, reward quality levels (QL), experience points, and credits.

Installation
------------

There's pretty much nothing needed to install MishRoller itself. Extrach all application files into a dedicated workspace directory. Upon first execution, MishRoller opens a folder search prompt. Navigate to your Project Rubi-Ka client installation folder and select the subfolder that contains the game binaries `Anarchy.exe` and `AnarchyOnline.exe`. MishRoller instantiates a hook to `AnarchyOnline.exe` on start. If a duplicate MishRoller is detected, the utility terminates the redundant process to eliminate crashes. You can however use a second mission roller such as clicksaver to verify the search results.

Usage
-----

The "missions" window Displays various data for each mission. It tracks playfield, coordinates, QL, reward item, find/return item, item(s) icon and credits. Items matching the set criteria are displayed in the color of your choosing. 

The "Search" window: Hosts item, location and mission type monitor watchlists. Here you can enter text to search among item names and locations or mission types. Ex: "battle suit", "hand axe", "Newlant" etc. You need to press the Apply button to set the changes

Item list: Checks if reward properties or descriptions trigger an item watchlist match, it allows for search-like constructors "<text>", -<text> and word-match.
Examples:
Searching for 'decus -gloves' will match all decus items except gloves;
Searching for 'decus armor' will match on 'decus body armor', and 'decus
armor boots'
Searching for '"decus armor"' will match on 'decus armor boots' but not on
'decus body armor'
Searching for '"primus decus" -gloves -boots -body' will match on all primus
decus armor except for gloves, boots and body
You can also search by quality, ex:
Searching for 'ql10 concrete -gl105' will match all concrete cushion items
at ql10 and ql100-109 but not ql105;
Searching for '"ql10 " concrete' will match only ql10 'concrete cushion'
if you want to search for ql95-ql105 item you need to split it in two
searches: 'ql9 decus -90 -91 -92 -93 -94' and 'ql10 decus -106 -107 -108 -109'
Remember that searching for QL depents on the mission level!

The "location list": You can use this to search for playfields (ie "Pleasant Meadows", "Broken Shores"...), as well as coordinates, or both.
Examples:
Searching for 'athen -shire' will match on 'west athens' and 'old athens'
Searching for 'athen (100-200,500-600)' will match on any athem mission with
coords x from 100 to 200, y from 500 to 600.
Searching for 'athen (100.2,200.3)' will match on any athen mission with
coords x and y exacly 100.2 and 200.3 respectively
Searching for 'athen (0-500,3000-999999)' will match on any athen mission
with coords x <=500, y>=3000 (but less than 999999)

In addition to the  buttons, you can press enter to add something in the list, The textbox will clear allowing you to type the next item and so on. You can also paste text directly into the list or the textbox and every line will be added as a new watch item.

The "Mission Type", flags, *Repair*, *Return Item*, *Find Person*, *Find Item*, and *Kill Person*. lets you select a type of mission you wish to be alarmed on.

Mission match conditions: If these are checked, a mission must have at least one item match to be selected.


The "Settings" window: Provides configuration for application parameters, font customization, background/button colors, mission sliders, and threshold parameters. You need to press the Apply button to set the changes.
Options:
 - Start Minimized: if this is checked, MishRoller start in minimized state.
 - Alert box: if this is checked, an alert box is displayed when one or more mission is found, depending on the mission match conditions (see below).
 - Select Match: This turns on/off the mouse movement to select the first matching mission.
 - Sounds: This turns on/off the sounds.. It plays found.wav and notfound.wav. notfount.wav won't play when running rolling agent.
 - Logging: This creates a file with various messages that happen while using the program.
 - Rolling Agent Help: if this is checked, an alert box is displayed when starting Rolling agent.
 - Auto Expant Team mission: This resizes the window to the right showing all team mission rewards.

Mission Sliders are used to set the mission parameters.

Item Values: Set the terminal multiplier, 4 is for all terminals, 5 for omni terminals and 7 for trader terminals.
Mission match conditions: If these are checked, a mission must match the item value or the total credits to be selected. Set it at a high value to disable it.

A mission must fulfill every active checkbox parameter simultaneously before an alert sounds or rolling is stopped.

Highlight: Toggle highlight checkboxes off inside your critera to selectively suppress highlighting (no color change) without disabling search rules. If you only want to highlight items for example.

The window is resizable horizontally. It's size and position are saved when exiting the program (as well as others config details and item watch list). Remember to press the Apply button!

You can minimize the window. It will maximize itself automatically when new missions are received.

Mish Rolling Agent
------------

This function rolls missions automatically until it finds a mission matching your mission search conditions. You have to have a mission terminal window already opened ingame and SELECTED, snap its window in the upper left corner of AO Window. Select your targeted base difficulty levels and slider configurations manually (you can use a preselected slider preset). In MishRoller's bottom control panel, input your retry limit into "Maximum Tries", specify your target missions count in "# of Mish", and click "Start Agent". The Agent will get new missions until either the maximum tries or the missions have been reached.

It will refuse to work if your search conditions and search list are setup in a way that means that no mission will ever match (eg. you're searching only missions with matching items, but item list is empty)
If you start the Agent with the mission booth window misplaced, it will wait forever for missions to appear, and it won't get out of this mode (which basically means that it won't display missions anymore and that it will move the mouse around every time you generate new missions from a mission terminal)

There's two things you can do in this case: put the mission booth window properly in the upper left corner, generate a mission and the buying agent will kick in.Or, quit MishRoller and launch it again. (use alt+tab or the taskbar to
show up MishRoller window again). MishRoller doesn't care about the placement of the AO window on the screen. Note also that clicking positions are preset and not configurable. It works by clicking twice on the leftmost pixel of the difficulty slider, then twice on the rightmost pixel of that slider, then it clicks on request missions. It means that if you use it with the difficulty slider either fully to the left or fully to the right, it will actually get off of one pixel or two from the position where you have put it.


History
-------
1.0.0 - Initial version based on ClickSaver 2.8.3 