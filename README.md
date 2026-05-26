# PRK MishRoller v1.0.2

A high-performance, native Win32 mission-rolling and packet-parsing utility custom-tailored for the **Project Rubi-Ka (Anarchy Online)** client. 

MishRoller hooks directly into client-side network streams to capture, parse, and display metadata for all terminal missions simultaneously. It features an integrated SQLite engine for real-time item lookups, advanced text-filtering matrices, and an automated agent to streamline mission rolling.

---

## 🚀 Features

* **Live Multi-Mission Grid**: Decodes incoming packet streams to instantly display Playfield, Coordinates, Quality Level (QL), Reward Item, Find Item, Icons, and Credits.
* **Automated Rolling Agent**: Simulates hardware inputs via a dedicated high-priority thread to auto-roll missions until user-defined search criteria are met.
* **Advanced Query Syntax**: Supports web-search-like syntax including strict phrase matching (`"decus armor"`), keyword exclusions (`-gloves`), and coordinate bounding boxes `(100-200,500-600)` for locations.
* **Slider Auto-Configuration**: Uses linear interpolation (`_linIinterp`) to automatically snap and drag all 7 client mission settings sliders based on 0-100% user preferences.
* **Smart Window Behaviors**: Auto-saves window boundaries and states to `LastSettings.mr`. Automatically shifts from minimized tray states to a maximized foreground display the moment new terminal packets arrive.

---

## 🛠️ Compilation & Requirements

MishRoller is written in optimized C/C++ using raw Win32 APIs, GDI, and the Windows Imaging Component (WIC) pipeline.

### Prerequisites
* **IDE**: Visual Studio 2013 or newer (VS2015, VS2019, VS2022 supported).
* **SDK**: Windows SDK (included natively with Visual Studio).
* **Dependencies**: SQLite3 (statically linked or included via local header/source bindings).

### Building via Visual Studio
1. Open Visual Studio and load the project solution (`.sln`).
2. Set the build configuration to **Release** and select your target architecture (typically **x86** to match the legacy game client ecosystem).
3. Ensure the character set configuration in Project Properties is configured appropriately for ANSI/Unicode string handling as dictated by the original source bounds.
4. Press `Ctrl + Shift + B` to compile the executable.

---

## 📦 Installation

1. **Standalone Folder**: Extract or copy the compiled binaries into a dedicated folder. MishRoller runs completely standalone and does not need to be dropped inside the game folder.
2. **Path Selection**: On its first run, the program opens a directory selection dialog. Navigate to your Project Rubi-Ka installation folder and target the directory containing `Anarchy.exe` and `AnarchyOnline.exe`.
3. **Database Hooking**: The application utilizes this file path to configure a local pipeline into `\cd_image\rdb.db`. 
4. **Single Instance Protection**: Launching a second instance of MishRollerautomatically brings the existing background application to the foreground and kills the duplicate process thread safely to prevent crashes.

---

## 💡 Usage

### Watchlist Navigation & Operations
Navigate to the **Search Window** (`ID_WINDOWS_SEARCH`) to establish monitoring for matches.
* `Return / Enter`: Commit the active textbox entry line to the monitoring matrix.
* `Ctrl + V (Clipboard Injection)`: Supports multi-line pasting to batch-import massive string lists.

### Advanced Query Filters
* **Exact Phrases**: Wrap target items inside double quotes (`"decus armor"`) to trigger strict spacing validation checks.
* **Exclusion Constraints**: Use a negative modifier prefix (`decus -gloves`) to parse matching properties while ignoring specified terms.
* **Quality Metrics**: Specify quality bounds directly inside text tokens (e.g., `ql10 concrete -ql105`). *Note: Filtering constraints depend explicitly on your current terminal tier.*
* **Coordinate Window Rules**: Append explicit boundary limits in parentheses to location fields (e.g., `athen (100-200,500-600)`). The layout parser reads telemetry data to flag missions within that specific geographic quadrant.

### Alert Frameworks
Set conditional alert rules inside the **Settings Window** using toggles:
* **Listed item is found**: Validates rewards or descriptions against the item watch list.
* **Listed location is found**: Compares target playfields and quadrants against coordinate criteria.
* **Mission Type Found**: Targets exact terminal configurations (*Repair*, *Return Item*, *Find Person*, *Find Item*, *Kill Person*).
* *Note: When multiple criteria are active, MishRoller implements logical AND conditions. An item must satisfy all active filters simultaneously to trigger an alert.*

---

## 🤖 Mission Rolling Agent

This Agent loops mouse click simulation macros automatically until your target filtering conditions are met.

### Operational Setup
1. Log in to your game client, access a standard mission terminal booth, and snap the window directly into the **upper-left corner** of your game display.
2. Configure your baseline slider preferences and target difficulty levels manually inside the game client.
3. Open MishRoller's control tray, specify your retry limits inside **Maximum Tries**, declare required counts inside **# of Mish**, and click **Start Agent**.

### Under the Hood
* **Simulation Loop**: Execution shifts onto a dedicated high-priority thread (`THREAD_PRIORITY_ABOVE_NORMAL`). It calculates spatial coordinate ranges relative to the client frame window, generating synchronized `WM_LBUTTONDOWN` and `WM_LBUTTONUP` messaging instructions.
* **Safety Lockouts**: The controller evaluates active watches before initiating mouse pointer tracks. If conditional locks are enabled but their respective list tables are entirely empty, the agent aborts execution immediately.
* **Slider Realignment**: Click **Set Sliders Now** to fire an internal linear interpolation framework (`_linIinterp`) that translates target percentages (0–100) directly into client coordinate layouts. The cursor rapidly slides handles across all 7 parameters: *Easy/Hard*, *Good/Bad*, *Order/Chaos*, *Open/Hidden*, *Physical/Mystical*, *Head On/Stealth*, and *Money/XP*.
