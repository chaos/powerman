####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
####################################################################
#   Copyright (C) 2001-2002 The Regents of the University of California.
#   Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
#   Written by Andrew Uselton (uselton2@llnl.gov>
#   UCRL-CODE-2002-008.
#   
#   This file is part of PowerMan, a remote power management program.
#   For details, see <http://www.llnl.gov/linux/powerman/>.
#   
#   PowerMan is free software; you can redistribute it and/or modify it under
#   the terms of the GNU General Public License as published by the Free
#   Software Foundation; either version 2 of the License, or (at your option)
#   any later version.
#   
#   PowerMan is distributed in the hope that it will be useful, but WITHOUT 
#   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
#   for more details.
#   
#   You should have received a copy of the GNU General Public License along
#   with PowerMan; if not, write to the Free Software Foundation, Inc.,
#   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
####################################################################
#-----tests for start-pm-1.sh---------------------------------------
#!/bin/bash
STAMP=`date`
#In all of the following you have to escape the glob special
# characters so the shell won't expand them.  The "set args ..."
# bit is what you say in gdb to put it on the command line.
#
# "set args -q" means "tell which plugs are on"
$PMDIR/bin/powerman -q 
#
# "set args -Q" means "tell which plugs are off"
$PMDIR/bin/powerman -Q
#
# "set args -1 tux1" means "turn on the plug for the node named tux1"
$PMDIR/bin/powerman -1 tux1
#
# "set args -q" see if it worked
$PMDIR/bin/powerman -q
#
# "set args -1v tux1" means "turn on the plug and verify it's operation"
# N.B. this uses the report option (-z), which hasn't been implemented yet
$PMDIR/bin/powerman -1v tux1
#
# "set args -0x tux[345]" means "turn off the plugs identified by the regex"
$PMDIR/bin/powerman -0x tux[345]
#
# "set args -q" see if it worked
$PMDIR/bin/powerman -q
#
# "set args -c tux6" means "power cycle the plug"
$PMDIR/bin/powerman -c tux6
#
# "set args -q" see if it worked
$PMDIR/bin/powerman -q
#
# "set args -rx tux[56]" means "reset the plugs (off nodes stay off)"
$PMDIR/bin/powerman -rx tux[56]
#
# "set args -q" see if it worked
$PMDIR/bin/powerman -q
#
# "set args -lx tux[3456]" means "list the matching nodes"
$PMDIR/bin/powerman -lx tux[3456]
#
# "set args -z" means "list the matching nodes"
$PMDIR/bin/powerman -z
#
echo $STAMP
date
exit
#-----tests for start-pm-16.sh---------------------------------------
# "set args -z" now showing 160 nodes
$PMDIR/bin/powerman -z
#
# "set args -1 \*5\*" everyone with a five is on
$PMDIR/bin/powerman -1 \*5\*
#
# "set args -z" now showing 160 nodes
$PMDIR/bin/powerman -z
# and the result looks like this:
# off             tux[0-4,6-14,16-24,26-34,36-44,46-49,60-64,66-74,76-84,86-94,96-104,106-114,116-124,126-134,136-144,146-149]
# on              tux[5,15,25,35,45,50-59,65,75,85,95,105,115,125,135,145,150-159]
# and its pretty snappy
#-----tests for start-pm-256.sh and start-pm-16x16.sh-----------------------
# "set args -z" now showing 160 nodes
$PMDIR/bin/powerman -z
#
# "set args -1 \*5\*" everyone with a five is on
$PMDIR/bin/powerman -1 \*5\*
#
# "set args -z" now showing 160 nodes
$PMDIR/bin/powerman -z
# and the result looks like this:
off             tux[0-4,6-14,16-24,26-34,36-44,46-49,60-64,66-74,76-84,86-94,96-
104,106-114,116-124,126-134,136-144,146-149,160-164,166-174,176-184,186-194,196-
204,206-214,216-224,226-234,236-244,246-249,260-264,266-274,276-284,286-294,296-
304,306-314,316-324,326-334,336-344,346-349,360-364,366-374,376-384,386-394,396-
404,406-414,416-424,426-434,436-444,446-449,460-464,466-474,476-484,486-494,496-
499,600-604,606-614,616-624,626-634,636-644,646-649,660-664,666-674,676-684,686-
694,696-704,706-714,716-724,726-734,736-744,746-749,760-764,766-774,776-784,786-
794,796-804,806-814,816-824,826-834,836-844,846-849,860-864,866-874,876-884,886-
894,896-904,906-914,916-924,926-934,936-944,946-949,960-964,966-974,976-984,986-
994,996-1004,1006-1014,1016-1024,1026-1034,1036-1044,1046-1049,1060-1064,1066-10
74,1076-1084,1086-1094,1096-1104,1106-1114,1116-1124,1126-1134,1136-1144,1146-11
49,1160-1164,1166-1174,1176-1184,1186-1194,1196-1204,1206-1214,1216-1224,1226-12
34,1236-1244,1246-1249,1260-1264,1266-1274,1276-1284,1286-1294,1296-1304,1306-13
14,1316-1324,1326-1334,1336-1344,1346-1349,1360-1364,1366-1374,1376-1384,1386-13
94,1396-1404,1406-1414,1416-1424,1426-1434,1436-1444,1446-1449,1460-1464,1466-14
74,1476-1484,1486-1494,1496-1499,1600-1604,1606-1614,1616-1624,1626-1634,1636-16
44,1646-1649,1660-1664,1666-1674,1676-1684,1686-1694,1696-1704,1706-1714,1716-17
24,1726-1734,1736-1744,1746-1749,1760-1764,1766-1774,1776-1784,1786-1794,1796-18
04,1806-1814,1816-1824,1826-1834,1836-1844,1846-1849,1860-1864,1866-1874,1876-18
84,1886-1894,1896-1904,1906-1914,1916-1924,1926-1934,1936-1944,1946-1949,1960-19
64,1966-1974,1976-1984,1986-1994,1996-2004,2006-2014,2016-2024,2026-2034,2036-20
44,2046-2049,2060-2064,2066-2074,2076-2084,2086-2094,2096-2104,2106-2114,2116-21
24,2126-2134,2136-2144,2146-2149,2160-2164,2166-2174,2176-2184,2186-2194,2196-22
04,2206-2214,2216-2224,2226-2234,2236-2244,2246-2249,2260-2264,2266-2274,2276-22
84,2286-2294,2296-2304,2306-2314,2316-2324,2326-2334,2336-2344,2346-2349,2360-23
64,2366-2374,2376-2384,2386-2394,2396-2404,2406-2414,2416-2424,2426-2434,2436-24
44,2446-2449,2460-2464,2466-2474,2476-2484,2486-2494,2496-2499]
on              tux[5,15,25,35,45,50-59,65,75,85,95,105,115,125,135,145,150-159,
165,175,185,195,205,215,225,235,245,250-259,265,275,285,295,305,315,325,335,345,
350-359,365,375,385,395,405,415,425,435,445,450-459,465,475,485,495,500-599,605,
615,625,635,645,650-659,665,675,685,695,705,715,725,735,745,750-759,765,775,785,
795,805,815,825,835,845,850-859,865,875,885,895,905,915,925,935,945,950-959,965,
975,985,995,1005,1015,1025,1035,1045,1050-1059,1065,1075,1085,1095,1105,1115,112
5,1135,1145,1150-1159,1165,1175,1185,1195,1205,1215,1225,1235,1245,1250-1259,126
5,1275,1285,1295,1305,1315,1325,1335,1345,1350-1359,1365,1375,1385,1395,1405,141
5,1425,1435,1445,1450-1459,1465,1475,1485,1495,1500-1599,1605,1615,1625,1635,164
5,1650-1659,1665,1675,1685,1695,1705,1715,1725,1735,1745,1750-1759,1765,1775,178
5,1795,1805,1815,1825,1835,1845,1850-1859,1865,1875,1885,1895,1905,1915,1925,193
5,1945,1950-1959,1965,1975,1985,1995,2005,2015,2025,2035,2045,2050-2059,2065,207
5,2085,2095,2105,2115,2125,2135,2145,2150-2159,2165,2175,2185,2195,2205,2215,222
5,2235,2245,2250-2259,2265,2275,2285,2295,2305,2315,2325,2335,2345,2350-2359,236
5,2375,2385,2395,2405,2415,2425,2435,2445,2450-2459,2465,2475,2485,2495,2500-255
9]
