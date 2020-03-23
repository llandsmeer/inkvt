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
export TESSDATA_PREFIX="data"

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

ORIG_FB_BPP="$(./fbdepth -g)"
echo "Original fb bitdepth is set @ ${ORIG_FB_BPP}bpp" >>crash.log 2>&1

case "${ORIG_FB_BPP}" in
    8) ;;
    16) ;;
    32) ;;
    *)
        unset ORIG_FB_BPP
        ;;
esac

ko_do_fbdepth() {
    if [ -n "${ORIG_FB_BPP}" ]; then
        echo "Switching fb bitdepth to 8bpp & rotation to Portrait" >>crash.log 2>&1
        ./fbdepth -d 8 -r -1 >>crash.log 2>&1
    fi
}

if awk '$4~/(^|,)ro($|,)/' /proc/mounts | grep ' /mnt/sd '; then
    mount -o remount,rw /mnt/sd
fi

if [ -e crash.log ]; then
    tail -c 500000 crash.log >crash.log.new
    mv -f crash.log.new crash.log
fi

sh ./enable-wifi.sh

sleep 10

CRASH_COUNT=0
CRASH_TS=0
CRASH_PREV_TS=0
# Because we *want* an initial fbdepth pass ;).
RETURN_VALUE=85
while [ ${RETURN_VALUE} -ne 0 ]; do
    # 85 is what we return when asking for a KOReader restart
    if [ ${RETURN_VALUE} -eq 85 ]; then
        # Do an update check now, so we can actually update KOReader via the "Restart KOReader" menu entry ;).
        # Do or double-check the fb depth switch, or restore original bitdepth if requested
        ko_do_fbdepth
    fi

    ./inkvt.armhf >> crash.log 2>&1
    RETURN_VALUE=$?

    if [ ${RETURN_VALUE} -ne 0 ] && [ ${RETURN_VALUE} -ne 85 ]; then
        CRASH_COUNT=$((CRASH_COUNT + 1))
        CRASH_TS=$(date +'%s')
        if [ $((CRASH_TS - CRASH_PREV_TS)) -ge 20 ]; then
            CRASH_COUNT=1
        fi

        if true; then
            ALWAYS_ABORT="true"
            CRASH_COUNT=1
        else
            ALWAYS_ABORT="false"
        fi

        viewWidth=600
        viewHeight=800
        FONTH=16
        eval "$(./fbink -e | tr ';' '\n' | grep -e viewWidth -e viewHeight -e FONTH | tr '\n' ';')"
        bombHeight=$((viewHeight / 2 + viewHeight / 15))
        bombMargin=$((FONTH + FONTH / 2))
        ./fbink -q -b -c -B GRAY9 -m -y 1 "Don't Panic! (Crash n°${CRASH_COUNT} -> ${RETURN_VALUE})"
        if [ ${CRASH_COUNT} -eq 1 ]; then
            ./fbink -q -b -O -m -y 2 "Tap the screen to continue."
        fi
        ./fbink -q -b -O -m -t regular=./fonts/freefont/FreeSerif.ttf,px=${bombHeight},top=${bombMargin} $'\xf0\x9f\x92\xa3'
        crashLog="$(tail -n 25 crash.log | sed -e 's/\t/    /g')"
        ./fbink -q -b -O -t regular=./fonts/droid/DroidSansMono.ttf,top=$((viewHeight / 2 + FONTH * 2 + FONTH / 2)),left=$((viewWidth / 60)),right=$((viewWidth / 60)),px=$((viewHeight / 64)) "${crashLog}"
        ./fbink -q -f -s
        {
            echo "!!!!"
            echo "Uh oh, something went awry... (Crash n°${CRASH_COUNT}: $(date +'%x @ %X'))"
            echo "Running FW $(cut -f3 -d',' /mnt/onboard/.kobo/version) on Linux $(uname -r) ($(uname -v))"
        } >>crash.log 2>&1
        if [ ${CRASH_COUNT} -lt 5 ] && [ "${ALWAYS_ABORT}" = "false" ]; then
            echo "Attempting to restart KOReader . . ." >>crash.log 2>&1
            echo "!!!!" >>crash.log 2>&1
        fi

        if [ ${CRASH_COUNT} -eq 1 ]; then
            read -r -t 15 </dev/input/event1
        fi
        CRASH_PREV_TS=${CRASH_TS}

        if [ ${CRASH_COUNT} -ge 5 ]; then
            echo "Too many consecutive crashes, aborting . . ." >>crash.log 2>&1
            echo "!!!! ! !!!!" >>crash.log 2>&1
            break
        fi

        if [ "${ALWAYS_ABORT}" = "true" ]; then
            echo "Aborting . . ." >>crash.log 2>&1
            echo "!!!! ! !!!!" >>crash.log 2>&1
            break
        fi
    else
        CRASH_COUNT=0
    fi
done

if [ -n "${ORIG_FB_BPP}" ]; then
    echo "Restoring original fb bitdepth @ ${ORIG_FB_BPP}bpp & rotation @ ${ORIG_FB_ROTA}" >>crash.log 2>&1
    ./fbdepth -d "${ORIG_FB_BPP}" -r "${ORIG_FB_ROTA}" >>crash.log 2>&1
else
    echo "Restoring original fb rotation @ ${ORIG_FB_ROTA}" >>crash.log 2>&1
    ./fbdepth -r "${ORIG_FB_ROTA}" >>crash.log 2>&1
fi

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
