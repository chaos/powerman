powerman-2.4.4 - 11 Sep 2024
----------------------------

Fix segfault affecting systems with power control hierarchy.

## Fixes

 * powerman: fix segfault if unspecified host reports status
 * redfishpower: do not report errors on dependent hosts
 * Update hostlist library to fix potential array out of bounds error.

powerman-2.4.3 - 11 Jul 2024
----------------------------

Command lines can be very long on a big system.

## Fixes

 * powerman: increase maximum line length

powerman-2.4.2 - 02 May 2024
----------------------------

More tuning for the large Cray EX system, and a work-around for
a libcurl bug on RHEL 8.

## New features

 * redfishpower: cache host resolution lookups (#190)
 * redfishpower: support new --resolve-hosts option (#188)
 * redfishpower: support message timeout config (#186)

## FIxes

 * redfishpower: increase default message timeout (#191)
 * redfishpower: output more detailed error messages (#183)


powerman-2.4.1 - 12 Apr 2024
----------------------------

This release represents a focused effort to support a large Cray EX system
including adding support in redfishpower to handle the power hierarchy
of Chassis/Blade/Node sensibly, and to better handle expected failure modes.

Powerman now supports the ability for a device script to match error output
and fail immediately.  Prior to this release, the only way to get powerman
to fail was to not provide expected successful output and run out the device
timeout.

## New features

 * redfishpower: support auth setup on command line (#181)
 * set default Cray EX authentication (#179)
 * add device file for Cray EX w/ Rabbit (#177)
 * add redfishpower HPE Cray EX chassis device file (#173)
 * powerman: support error diagnostics with setresult (#172)
 * redfishpower: add more details on hierarchy errors (#174)
 * powerman: support new setresult directive (#168)
 * powerman: use singlet script if targeting one plug (#170)
 * redfishpower: support plug parents (#164)
 * redfishpower: support plug substitution (#159)
 * redfishpower: support setpath configuration  (#158)
 * redfishpower: support setplugs configuration (#157)
 * redfishpower: refactor internals to use plugs (#160)
 * redfishpower: always do off/delay/on for power cycle (#149)
 * redfishpower: send http request after cmd active (#146)
 * powermand: don't daemonize and drop -f,--foreground option (#141)
 * libczmq: add containers from the CZMQ project (#124)

## Fixes

 * redfishpower: adapt status polling interval (#167)
 * redfishpower: fix memleaks and test under valgrind (#169)
 * powerman: when status and status_all are defined, use status_all only
   on full pluglist (#156)
 * redfishpower: add extra timeout debug information (#154)
 * redfishpower: adjust verbosity output (#151)
 * reduce log noise (#140)
 * don't allocate a pseudo-terminal for each coprocess (#135)
 * redfishpower: handle http 400 error (#132)

## Cleanup

 * drop antiquated memory protection magic (#136)
 * redfishpower: cleanup & refactoring (#134)

## CI/Test/build system

 * configure.ac: build helper executables by default (#180)
 * test a huge cray-ex configuration (#127)
 * redfishpower: reduce polling interval in test mode (#155)
 * redfishpower: add option to test host errors  (#145)
 * redfishpower: support test mode (#143)
 * enable valgrind test with suppressions (#137)

powerman-2.4.0 - 06 Feb 2024
----------------------------

This release is the result of a concentrated cleanup and modernization
effort.  The minor version was incremented because some options have
changed which could affect scripts that drive powerman.

## New features
 * systemd: run as Type=simple (#114)
 * redfishpower: output extra error info (#97)
 * systemd: allow group to be configured and set SHELL in env; add UBNT
   edge device (#96)
 * redfishpower: allow timeout to be set by device script (#72)

## Fixes
 * powermand: fix assertion failure on teardown (#118)
 * etc: fix logic error in redfishpower cray windom (#70)
 * redfishpower: check for post data (#66)

## Documentation
 * improve --device documentation and testing (#116)
 * Add license text to header files (#93)
 * Update license headers to SPDX license identifier (#92)

## Cleanup
 * redfishpower: remove --hostsfile option (#123)
 * redfishpower: minor cleanup (#117)
 * cull unused test options and update manual pages (#112)
 * clean up powerman client options (#113)
 * improve the powerman client's usage/help output, and minor source cleanup
   (#95)
 * reorganize project directories (#86)
 * drop trailing whitespace from configs, etc (#82)
 * Fix misleading-indentation error when running make on RHEL9 (#65)
 * systemd: avoid hardcoded paths and locate pid file under /run (#62)

## CI/Test/build system
 * testsuite: add valgrind coverage (#120)
 * testsuite: add clarification to sierra test script (#121)
 * convert remaining tests to sharness (#111)
 * convert more tests to sharness (#109)
 * convert old school power control box tests to sharness (#108)
 * convert still more tests to the sharness framework (#104)
 * convert more tests to use the sharness framework (#103)
 * convert several tests to use the sharness framework (#102)
 * testsuite: add sharness scripts (#98)
 * add test deb packaging and fix misc build problems (#91)
 * testsuite: use TAP for unit tests (#87)
 * mergify: fix approved-reviews-by typo (#90)
 * .mergify.yml: Add mergify support (#89)
 * build: modernize autoconf, fix bison/flex detection (#84)
 * test: fix redfishpower tests (#71)
 * testsuite: fix parallel make failure (#64)
 * require warning-free compilation (#61)

powerman-2.3.27 - 14 Dec 2021
-----------------------------

* Add redfish support for Cray r272z30, Cray windom, and Supermicro
  H12DSG-O-CPU (#55, #47)

* CI: Enable github workflow (#59, #58, #57, #56)

* Misc fixes (#54, #52, #50, #46)

powerman-2.3.26 - 18 Feb 2020
-----------------------------

* Log power state changes to syslog (Olaf Faaland, PR #37)

* Fix default systemd unit file path for 'make distcheck'

powerman-2.3.25 - 28 Jan 2019
-----------------------------

* Add etc/rancid-cisco-poe.dev (Daniel Rich, PR #28)

* Add etc/openbmc.dev (Albert Chu, PR #33)

* Add etc/kvm.dev & etc/kvm-ssh.dev (tisbeok, PR #8)

* Fix misinterpretation of error strings in ipmipower.dev.

powerman-2.3.24 - 23 Oct 2015
-----------------------------

* Don't package /var/run/powerman; let systemd manage it [TOSS-2987]

* Cleanup: drop trailing whitespace

powerman-2.3.23 - 08 Jun 2015
-----------------------------

* Build: silence CC lines, fix AC_LANG_CONFTEST warnings, fix $(EXEEXT)
  warnings.

* Build: install System V init scripts if --with-systemdsystemunitdir
  is not configured and include both in EXTRA_DIST.

* Build: re-enable 'make check' unit tests.

* Build: fix some 'make distcheck' issues, but until unit tests are fixed
  to find *.exp and *.conf files in $(srcdir), this will still fail.

* RPM: configure genders, httppower, snmppower, and tcp-wrappers
  unconditionally; update URL.

powerman-2.3.22 - 01 Jun 2015
-----------------------------

* Assorted build system fixes to allow 'make distcheck'
  and Koji builds on RHEL 7 to work.

* Note: unit tests temporarily disabled pending rework.

[This release was not distributed as it was incomplete for RHEL 7/Koji]

powerman-2.3.21 - 29 May 2015
-----------------------------

* Added systemd unit file (bacaldwell, gc issue #42)
  SystemVinit script support is dropped.

* Build against -lnetsnmp not -lsnmp (TOSS-2815)

* Add raritan-px5523.dev (Daryl Granau, TOSS-2486)

powerman-2.3.20 - 26 Aug 2014
-----------------------------

* add dist tag to release (TOSS-2667)

* Minor automake updates

powerman-2.3.19 - 25 Aug 2014
-----------------------------

* Added apc8941.dev (TOSS-2658, Tim Randles)

powerman-2.3.18 - 21 Aug 2014
-----------------------------

* Stop tracking autotools products

* Add README.md for github move

powerman-2.3.17 - 27 Mar 2013
-----------------------------

* Fix powerman-stonith script to handle aliased plugs and
  add regression testing for it (TOSS-1962)

powerman-2.3.16 - 04 Oct 2012
-----------------------------

* Fix duplicate node name (issue 35)
  Pulled in another hostlist fix (Mark Grondona)

* Fix powerman stonith OFF should verify plug state (chaos bz 1439)

powerman-2.3.15 - 05 Sep 2012
-----------------------------

* Added ipmipower-serial.dev [Al Chu]

powerman-2.3.14 - 10 Aug 2012
-----------------------------

* Fix issue 34: duplicate node name in configuration file
  Updated hostlist code to the latest which fixes this issue.

powerman-2.3.13 - 20 Jun 2012
-----------------------------

* Updated appro-gb2.dev per Appro

* Add support for Baytech RPC22 (Olof Johansson)

powerman-2.3.12 - 13 Jan 2012
-----------------------------

* Add support for Raritan px4316 (chaos bz 1276)

* Add support for DLI web power switches III and IV [Gaylord Holder]

* Add --single-cmd option to plmpower (issue 7) [Ira Weiny]

* Minor documentation updates

powerman-2.3.11 - 01 Sep 2011
-----------------------------

* Update appro-gb2.dev (chaos bug 1218)

* Fix BuildRequires for tcp_wrappers-devel to work on el6/ch5.
  Packagers: now you must add --with-tcp-wrappers on configure line
  to enable this feature (before it was enabled if libs were present)

* Re-autogen to pull in (new?) bison build dependency on el6/ch5.

* Fix --with-feature options to fail if prereqs are missing, not just
  silently disable the feature.

* Fix line number accounting during parse error reporting (issue 3)

powerman-2.3.10 - 19 Aug 2011
-----------------------------

* Updated appro-greenblade.dev (chaos bug 1218)

* Added appro-gb2.dev (chaos bug 1218)

* Added sentry_cdu.dev (chaos bug 1218)

* Added baytech-rpc18d-nc (issue 5)

powerman-2.3.9 - 25 Feb 2011
----------------------------

* Add MIB support to snmppower.

* Add eaton-epdu-blue-switched.dev [Paul Anderson].

powerman-2.3.8 - 24 Feb 2011
----------------------------

* Add support for SNMP power controllers via 'snmppower' helper.

* Add SNMP dev files for 8-port APC, 8-port Baytech, and 20 port
  Eaton Revelation PDU.

powerman-2.3.7 - 04 Nov 2010
----------------------------

* Add support for APC 7900 revision 3 firmware [Py Watson]

* Internal automake cleanup.

powerman-2.3.6 - 12 Aug 2010
----------------------------

* Convert several internal buffers from static to dynamic to address
  overflow in query output [chaos bugzilla 1009]

* Add support for Appro Greenblade [Trent D'Hooge].

* Add support for APC 7920 [Manfred Gruber].

* Add Support for ranged beacon on/off device scripts, and beacon
  support for ipmipower [Al Chu].

powerman-2.3.5 - 17 Apr 2009
----------------------------

* Deprecated undocumented powerman.conf port directive.

* Added powerman.conf listen directive to configure which interfaces
  and ports the server listens on.  Make the default localhost:10101.

* Add support for HP integrated power control devices [Bjorn Helgaas]

* Add support for Sun LOM.

* Misc. documentation improvements

* Add heartbeat STONITH plugin.

powerman-2.3.4 - 09 Feb 2009
----------------------------

* Fix powerman-controlling-powerman config so that status command
  is fast for large configs again.

* Fix "bashfun" device script and add regression test.

* Various changes coming from Debian audit [Arnaud Quette].

powerman-2.3.1 - 01 Dec 2008
----------------------------

* Initial powerman client API.

* Run the powerman daemon (and all coprocesses) as 'daemon' by default
  instead of root.  To override, set USER=root or other user in
  /etc/sysconfig/powerman.

* New man pages for httppower, plmpower, libpowerman, vpcd.

* Include vpcd in the dist.

* Various changes coming from Debian audit [Arnaud Quette].

powerman-2.3 - 12 Nov 2008
--------------------------

* Make init script work on Solaris.

* IPv6 support.

* Include example powerman.conf file (/etc/powerman/powerman.conf.example)

* Added support for Cyclades PM20, PM42.

powerman-2.2 - 27 Aug 2008
--------------------------

* Send a SIGINT to coprocesses rather than just closing file
  descriptors during powermand shutdown.  This is required to 
  terminate an ssh client running in coprocess mode.

* Make it possible to only define the _ranged version of scripts.  Scripts 
  are selected using the following precedence: _all, _ranged, singlet.

* Drop "soft power status" support.  This allowed iceboxes to detect 
  whether a node was powered up or not, independently of plug state.

* [ipmipower] Drop _all and singlet version of scripts. 

* [ilom] Revised Sun Integrated Lights Out Management support 
  to work over ssh and serial.  Dropped shared console/serial 
  "ilom-inline" support.

* [icebox3] Now supports both v3 and v4 Ice Box.  Users of icebox4
  should change their device types to icebox3 in powerman.conf.

* [plmpower] Added support for controlling Insteon/X10 devices via 
  SmartLabs PLM 2412S.

* [hp3488] Added support for modular GPIB-based HP 3488A switch/control 
  unit via gpib-utils project.

* [ics8064] Added support for VXI-11-based ICS 8064 16-port relay unit
  via gpib-utils project.

* [powerman] Improved efficiency of powerman-controlling-powerman 
  configurations, when one powerman controls a subset of the plugs of 
  another powerman through the use of _ranged scripts.

* Enhanced integrated test suite.
  Note: tests all pass on AIX now.

powerman-2.1 - 18 Jun 2008
--------------------------

* Client overhaul.

* [apcpdu3] Fix intermittent query failures.

* Fixed off by one bug in server that allowed some delays in dev 
  scripts to take longer than programmed.

* Use raw mode in pipe (|&) connections to avoid local tty echo.

* Run server in /var/run/powerman rather than /etc/powerman.
  Create this directory if it doesn't exist.

* Enhanced integrated test suite.
  Note: all tests do not currently pass on AIX.

powerman-2.0 - 01 Jun 2008
--------------------------

* Autoconf/automake integration.

* Integrated test suite.

* Portability to Solaris, AIX, and OS/X.

* Support for selecting power control targets using genders (-g option).

* Cleanup and refactoring.

powerman-1.0.32 - 24 Jan 2008
-----------------------------

* Support for new apc firmware (apcpdu3.dev) [Trent D'Hooge]

* Added httppower utility for interfacing to power controllers that are
  exclusively web-based.

* Support for Digital Loggers, Inc. power controllers.

* Support for APPRO power controller.

powerman-1.0.31 - 20 Oct 2007
-----------------------------

(Includes changes in 1.0.29 and 1.0.30)

* Support SUN ILOM inline with serial console, i.e.
  device "ilom25" "ilom-inline" "/usr/bin/conman -Q -j ilc25 |&"

* Handle config files with embedded carriage returns [Trent D'Hooge
  and Todd Heer].

* Ipmipower device script tuning [Al Chu].

* Minor build/packaging tweaks for building under mock, etc.

powerman-1.0.28 - 07 May 2007
-----------------------------

* Increase "cycle" delays from 2s to 4s on all devices that implement
  cycle as an explicit on,delay,off.

powerman-1.0.27 - 24 Apr 2007
-----------------------------

* Add heartbeat stonith module.

* Fix bug in baytech-rpc28-nc device support that affected plugs > 10.

powerman-1.0.26 - 21 Dec 2006
-----------------------------

* Support 8-port APC AP7900 and likely AP7901, AP7920, and AP7921.
  [Martin Petersen]

* Work around a bug that causes baytech rpc3's to return some plugs
  as status "unknown" on login timeout/reconnect.

* Add ranged power control support for faster power control w/ ipmipower.

powerman-1.0.25 - 16 Aug 2006
-----------------------------

* Support newer baytech RPC-3 firmware.

* Support icebox v4 [Jarod Wilson]

powerman-1.0.24 - 30 May 2006
-----------------------------

* Telnet state machine now works with Digi terminal server in telnet 
  mode and logs ignored telnet option requests.

* Several powerman.conf examples are now included in the RPM doc area.

* Handle expansion of suffixed host ranges, e.g. "[00-26]p" [Mark Grondona]

* Fix minor memory leaks and unchecked function return values 
  found by Coverity.

* New powerman.dev(5) man page documenting device file syntax.

* Support serial device baud rates up to 460800 baud if system does.

* Minor tweaks to spec file for Fedora Extras [Ben Woodard].

* Fix for broken P0|P1 in Rackable Phantom 4.0 firmware (phantom4.dev).

* Add support for IBM BladeCenter chassis (ibmbladecenter.dev) 
  [Robin Goldstone].

* Add support for ComputerBoards CB7050 (cb-7050.dev).

* Add support for Cyclades PM10 (cyclades-pm10.dev) [Trent D'Hooge].

powerman-1.0.23 - 16 Feb 2006
-----------------------------

* Fix for compilation on Fedora Core 4 [Ben Woodard].

* Spec file changes for Fedora Extras submission [Ben Woodard].

powerman-1.0.22 - 12 Oct 2005
-----------------------------

* Bug fix for powerman -T dumps core on x86_64 [Thayne Harbaugh]

* Add support for 24-port APC Switched Rack PDU (apcpdu.dev) [Makia Minnich].
