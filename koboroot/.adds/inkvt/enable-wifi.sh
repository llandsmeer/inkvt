#!/bin/sh
# From https://github.com/koreader/koreader/blob/master/platform/kobo/enable-wifi.sh
# Copyright (C) 2020 koreader
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Load wifi modules and enable wifi.

lsmod | grep -q sdio_wifi_pwr || insmod "/drivers/${PLATFORM}/wifi/sdio_wifi_pwr.ko"
# Moar sleep!
usleep 250000
# WIFI_MODULE_PATH = /drivers/$PLATFORM/wifi/$WIFI_MODULE.ko
lsmod | grep -q "${WIFI_MODULE}" || insmod "${WIFI_MODULE_PATH}"
# Race-y as hell, don't try to optimize this!
sleep 1

ifconfig "${INTERFACE}" up
[ "${WIFI_MODULE}" != "8189fs" ] && [ "${WIFI_MODULE}" != "8192es" ] && wlarm_le -i "${INTERFACE}" up

pkill -0 wpa_supplicant ||
    env -u LD_LIBRARY_PATH \
        wpa_supplicant -D wext -s -i "${INTERFACE}" -c /etc/wpa_supplicant/wpa_supplicant.conf -O /var/run/wpa_supplicant -B

# And finally, request an IP ourselves, because we killed Nickel's master dhcpcd process
dhcpcd -d -t 30 -w "${INTERFACE}"
