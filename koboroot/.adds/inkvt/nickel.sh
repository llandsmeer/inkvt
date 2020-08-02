#!/bin/sh
# From https://github.com/koreader/koreader/blob/master/platform/kobo/nickel.sh
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

PATH="/sbin:/bin:/usr/sbin:/usr/bin:/usr/lib:"

export LD_LIBRARY_PATH="/usr/local/Kobo"

cd /
unset OLDPWD LC_ALL

(
    if [ "${PLATFORM}" = "freescale" ] || [ "${PLATFORM}" = "mx50-ntx" ] || [ "${PLATFORM}" = "mx6sl-ntx" ]; then
        usleep 400000
    fi
    /etc/init.d/on-animator.sh
) &

if lsmod | grep -q sdio_wifi_pwr; then
    killall -q -TERM enable-wifi.sh
    cp -a "/etc/resolv.conf" "/tmp/resolv.ink"
    dhcpcd -d -k "${INTERFACE}"
    killall -q -TERM udhcpc default.script
    mv -f "/tmp/resolv.ink" "/etc/resolv.conf"
    wpa_cli terminate
    [ "${WIFI_MODULE}" != "8189fs" ] && [ "${WIFI_MODULE}" != "8192es" ] && wlarm_le -i "${INTERFACE}" down
    ifconfig "${INTERFACE}" down
    usleep 250000
    rmmod "${WIFI_MODULE}"
    usleep 250000
    rmmod sdio_wifi_pwr
fi

sync

/usr/local/Kobo/hindenburg &
LIBC_FATAL_STDERR_=1 /usr/local/Kobo/nickel -platform kobo -skipFontLoad &
udevadm trigger &

if [ -e "/dev/mmcblk1p1" ]; then
    echo sd add /dev/mmcblk1p1 >>/tmp/nickel-hardware-status &
fi

return 0
