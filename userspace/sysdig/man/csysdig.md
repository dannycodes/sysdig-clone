NAME
----

csysdig - the ncurses user interface for sysdig

SYNOPSIS
--------

**csysdig** [*option*]... [*filter*]

DESCRIPTION
-----------

csysdig exports sysdig's functionality through an intuitive and powerful ncurses-based user interface.

csysdig has been designed to mimic tools like **top** and **htop**, but it offers richer functionality, including:

- Support for both live analysis and sysdig trace files. Trace files can come from the same machine or from another machine. 
- Visibility into a broad range of metrics, including CPU, memory, disk I/O, network I/O.
- Ability to observe input/output activity for processes, files, network connections and more.
- Ability to drill down into processes, files, network connections and more to further explore their behavior.
- Full customization support.
- Support for sysdig's filtering language.
- Container support by design.

csysdig works on any terminal, and has support for colors and mouse input.

**Views**  

csysdig is based on the concept of 'views', little Lua scripts that determine how metrics are collected, processed and represented on screen. Including a new visualization to csysdig doesn't require to update the program, and is simply a matter of adding a new view. Views rely on the sysdig processing engine, and this means that they can include any sysdig filter field. Views are located in the sysdig chisel directory path, usually */usr/share/sysdig/chisels* and *~/.chisels*.


BASIC USAGE
-----------

Here are some basic tips to get you started with sysdig:

1. If you run csysdig without arguments, it will display live system data, updating every 2 seconds. To analyze a trace file, use the -r command line flag.
2. You can switch to a different view by using the _F2_ key.
3. You can to drill down into a selection by clicking _enter_. You can navigate back by typing _backspace_.
4. You can observe input/output for the currently selected entity by typing _F5_
5. You can see sysdig events for the currently selected entity by typing _F6_

DRILLING DOWN
-------------
You drill down by selecting an element in a view and then clicking _enter_. Once inside a selection, you can switch to a different view, and the new view will be applied in the context of the selection. For example, if you drill down into a process called foo and then switch to the _Connections_ view, the output will include only the connections made or received by _foo_. 

You can drill down multiple times, by keeping clicking _enter_. For example, you can click on a container in the _Containers_ view to get the processes running inside it, and then click on one of the processes to see its threads.

ACTIONS AND HOTKEYS
-------------------
Each view has a list of command lines that can be executed in the context of the current selction by pressing 'hotkeys'. For example, pressing 'k' in the Processes view kills the selected process, pressing 'b' in the Containers view opens a bash shell in the selected container. 

Each view supports different actions. You can see which actions a view supports by pressing F8. You can customize the view's actions by editing the view's Lua file. 

CONTAINERS SUPPORT
------------------
Starting csysdig with the -pc command line switch will cause many of the views to include additional container information. For example, the _Processes_ will include the columns with the container the process belongs to. Similarly, the _Connections_ view will show which container each connection belongs to.

INTERACTIVE COMMANDS  
--------------------  

##Views Window##

**Arrows, PgUP, PgDn, Home, End**  
  Change the selection and scroll view content, both vertically and horizontally.  

**Enter**  
  Drill down into the currently highlighted entry.  

**Backspace**  
  Navigate back to the previous view.  

**F2**  
  Show the view picker. This will let you switch to another view.  

**CTRL+F /**  
  Incremental search in the list of view entries.  

**F4**  
    Incremental filtering of the view entries.  

**F5, e**  
  'echo FDs' for the selection, i.e. view FD input/output for the currently highlighted entry.  

**F6, d**  
  'dig' into the selection, i.e. view sysdig events for the currently highlighted entry. Refer to the sysdig man page to learn about interpreting the content of this window.  

**F7**  
  Show the help page for the currently displayed view.  

**F8**  
  Open the view's actions panel.  

**F9, >**  
  Open the column sort panel.  

**F10, q**  
  Quit.  

**DEL, c**  
  For views that are listing elements without aggregating them by key (identifiable by yellow column headers), this command clears the view content.  

**p**  
  Pause screen updates.  

**`<shift> <1-9>`**  
  sort column `<n>`  

**F1, h, ?**  
  Show the help screen.  

##Echo and sysdig Windows##

**Arrows, PgUP, PgDn, Home, End**  
  Scroll the page content.  

**Backspace**  
  Navigate back to the previous view.  

**CTRL+F /**  
  Search inside the window content.  

**F3**  
  Find Next.  

**F2**  
  Chose the output rendering format. Options are 'Dotted ASCII' (non-printable binary bytes are rendered as dots), 'Printable ASCII' (non-printable binary bytes are not included and line endings are rendered accurately) and 'Hex' (dotted ASCII representation is included together with the Hexadecimal rendering of the buffers).

**DEL, c**  
  Clear the screen content.  

**p**  
  Pause screen updates.  

**CTRL+G**  
  Go to line.  

##Spectrogram Window##

**F2**  
  Show the view picker. This will let you switch to another view.  

**p**  
  Pause/Resume the visualization.  

**Backspace**  
  Navigate back to the previous view.  

MOUSE USAGE
-----------
- Clicking on column headers lets you sort the table.
- Double clicking on row entries performs a drill down.
- Clicking on the filter string at the top of the screen (the text after 'Filter:') lets you change the sysdig filter and customize the view content.
- You can use the mouse on the entries in the menu at the bottom of the screen to perform their respective actions.

COMMAND LINE OPTIONS
--------------------
  
**-d** _period_, **--delay**=_period_  
  Set the delay between updates, in milliseconds (by default = 2000). This works similarly to the -d option in top.  

**-E**, **--exclude-users**  
  Don't create the user/group tables by querying the OS when sysdig starts. This also means that no user or group info will be written to the tracefile by the -w flag. The user/group tables are necessary to use filter fields like user.name or group.name. However, creating them can increase sysdig's startup time.  

**-h**, **--help**  
  Print this page

**-k**, **--k8s-api**
  Enable Kubernetes support by connecting to the API server specified as argument. E.g. "http://admin:password@127.0.0.1:8080". The API server can also be specified via the environment variable SYSDIG_K8S_API.

**-K** _filename_, **--k8s-api-cert=**_filename_
  Use the provided certificate file name to authenticate with the K8S API server. Filename must be a full absolute or relative (to the current directory) path to the certificate file. The certificate can also be specified via the environment variable SYSDIG_K8S_API_CERT.

**-l**, **--list**  
  List all the fields that can be used in views.
  
**--logfile** _file_  
  Print program logs into the given file.

**-N**
  Don't convert port numbers to names.
  
**-n** _num_, **--numevents**=_num_  
  Stop capturing after _num_ events

**-pc**, **-pcontainers**_  
  Instruct csysdig to use a container-friendly format in its views. This will cause several of the views to contain additional container-related columns.

**-r** _readfile_, **--read**=_readfile_  
  Read the events from _readfile_.
  
**-s** _len_, **--snaplen**=_len_  
  Capture the first _len_ bytes of each I/O buffer. By default, the first 80 bytes are captured. Use this option with caution, it can generate huge trace files.

**-v** _view_id_, **--views**=_view_id_  
  Run the view with the given ID when csysdig starts. View IDs can be found in the view documentation pages in csysdig. Combine this option with a command line filter for complete output customization.

**--version**  
  Print version number.

FILTERING
---------
Similarly to what you do with sysdig, you can specify a filter on the command line to restrict the events that csysdig processes. To modify the filter while the program is running, or to add a filter at runtime, click on the filter text in the UI with the mouse.

CUSTOMIZING CSYSDIG
-------------------
csysdig is completely customizable. This means that you can modify any of the csysdig views, and even create your own views. Like sysdig chisels, csysdig views are Lua scripts. Full information can be found at the following github wiki page: https://github.com/draios/sysdig/wiki/csysdig-View-Format-Reference.

FILES
-----

*/usr/share/sysdig/chisels*  
  The global views directory.

*~/.chisels*  
  The personal views directory.

AUTHOR
------

Draios Inc. (dba Sysdig) <info@sysdig.com>

SEE ALSO
--------

**sysdig**(8), **strace**(8), **tcpdump**(8), **lsof**(8)
