OBO SD-backed web interface
===========================

Included changes
----------------
- Keeps the latest 50 messages in a ring buffer.
- Viewing or refreshing the page does not clear the messages.
- Serves separate home and settings pages from INDEX.HTM and SETTINGS.HTM.
- Serves the shared STYLE.CSS and APP.JS assets from the SD card.
- Allows the device IPv4 address, MAC address, settings username and settings
  password to be changed.
- Protects every configuration change with the current username and password.
- Removes UDP, local-port, remote-IP and remote-port configuration completely.
- Uses a responsive modern dashboard with a top navigation bar.
- Keeps monitoring on / and moves protected administration to /settings.

Installation
------------
1. Replace the project's ether_printer.h with the included ether_printer.h.
2. Make sure client_loop(); is called near the start of the main loop().
   The included obo_mega_legacy_web.ino already contains this call.
3. Copy these files to the root of the SD card:
     INDEX.HTM
     SETTINGS.HTM
     STYLE.CSS
     APP.JS
     DATA.TXT
4. Open http://<device-ip>/ for monitoring.
5. Open http://<device-ip>/settings for protected configuration.

DATA.TXT format
---------------
The current file contains exactly four lines:

  device IPv4 address
  device MAC address
  settings username
  settings password

Example:

  192.168.1.177
  DE:AD:BE:EF:FE:E0
  admin
  admin

The MAC address accepts six hexadecimal byte pairs separated by colons or
hyphens. It is saved in uppercase colon-separated form.

Migration
---------
The header can read and migrate both previous layouts:

- Three lines: IP, username, password.
- Six lines: IP, local port, remote IP, remote port, username, password.

For those older layouts, the device keeps the saved IP and credentials, uses
the default MAC DE:AD:BE:EF:FE:E0, removes all obsolete remote/port values,
and rewrites DATA.TXT into the new four-line format.

Network changes
---------------
After a valid IP or MAC change is saved, the HTTP response is completed first.
The Ethernet interface is then restarted with the new values. An IP change
redirects the page to the new address; a MAC-only change reloads the same page.

Notes
-----
- INDEX.HTM and SETTINGS.HTM use FAT 8.3-compatible filenames for classic
  Arduino SD libraries.
- Credentials are stored as plain text on the SD card and transmitted over
  HTTP. Use the interface only on a trusted local network.
- Long blocking delays in the legacy sketch can temporarily delay web replies.
- Message history is retained in RAM until the Arduino is restarted.
- Assign a unique MAC address to every device on the same Ethernet network.
