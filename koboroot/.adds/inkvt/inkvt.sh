#!/bin/sh
# From https://github.com/koreader/koreader/blob/master/platform/kobo/koreader.sh
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


export LC_ALL="en_US.UTF-8"

INKVT_DIR="${0%/*}"

cd "${INKVT_DIR}" || exit

export FROM_NICKEL="false"
if pkill -0 nickel; then
    FROM_NICKEL="true"
fi

if [ "${FROM_NICKEL}" = "true" ]; then
    FROM_KFMON="false"
    if pkill -0 kfmon; then
        if [ "$(pidof kfmon)" -eq "${PPID}" ]; then
            FROM_KFMON="true"
        fi
    fi
    eval "$(xargs -n 1 -0 <"/proc/$(pidof nickel)/environ" | grep -e DBUS_SESSION_BUS_ADDRESS -e NICKEL_HOME -e WIFI_MODULE -e LANG -e WIFI_MODULE_PATH -e INTERFACE 2>/dev/null)"
    export DBUS_SESSION_BUS_ADDRESS NICKEL_HOME WIFI_MODULE LANG WIFI_MODULE_PATH INTERFACE
    sync
    killall -TERM nickel hindenburg sickel fickel fmon 2>/dev/null
fi

if [ -z "${PRODUCT}" ]; then
    PRODUCT="$(/bin/kobo_config.sh 2>/dev/null)"
    export PRODUCT
fi

if [ -z "${PLATFORM}" ]; then
    PLATFORM="freescale"
    if dd if="/dev/mmcblk0" bs=512 skip=1024 count=1 | grep -q "HW CONFIG"; then
        CPU="$(ntx_hwconfig -s -p /dev/mmcblk0 CPU 2>/dev/null)"
        PLATFORM="${CPU}-ntx"
    fi

    if [ "${PLATFORM}" != "freescale" ] && [ ! -e "/etc/u-boot/${PLATFORM}/u-boot.mmc" ]; then
        PLATFORM="ntx508"
    fi
    export PLATFORM
fi

if [ -z "${INTERFACE}" ]; then
    INTERFACE="eth0"
    export INTERFACE
fi

ORIG_FB_ROTA="$(cat /sys/class/graphics/fb0/rotate)"
echo "Original fb rotation is set @ ${ORIG_FB_ROTA}" >>crash.log 2>&1

ORIG_FB_BPP="$(cat /sys/class/graphics/fb0/bits_per_pixel)"
echo "Original fb bitdepth is set @ ${ORIG_FB_BPP}bpp" >>crash.log 2>&1

if [ -e crash.log ]; then
    tail -c 500000 crash.log >crash.log.new
    mv -f crash.log.new crash.log
fi

sh ./enable-wifi.sh
sleep 10

echo "Switching fb bitdepth to 8bpp & rotation to Portrait" >>crash.log 2>&1
./fbdepth -d 8 -r -1 >>crash.log 2>&1

./inkvt.armhf >> crash.log 2>&1
RETURN_VALUE=$?

echo "Restoring original fb bitdepth @ ${ORIG_FB_BPP}bpp & rotation @ ${ORIG_FB_ROTA}" >>crash.log 2>&1
./fbdepth -d "${ORIG_FB_BPP}" -r "${ORIG_FB_ROTA}" >>crash.log 2>&1

if [ "${FROM_NICKEL}" = "true" ]; then
    if [ "${FROM_KFMON}" != "true" ]; then
        ./nickel.sh &
    else
        ./nickel.sh &
    fi
else
    if ! pgrep -f kbmenu >/dev/null 2>&1; then
        /sbin/reboot
    fi
fi

exit ${RETURN_VALUE}
