#!/bin/sh
PATH="/sbin:/bin:/usr/sbin:/usr/bin:/usr/lib:"

export LD_LIBRARY_PATH="/usr/local/Kobo"

cd /
unset OLDPWD EXT_FONT_DIR TESSDATA_PREFIX FROM_NICKEL STARDICT_DATA_DIR LC_ALL KO_NO_CBB

(
    if [ "${PLATFORM}" = "freescale" ] || [ "${PLATFORM}" = "mx50-ntx" ] || [ "${PLATFORM}" = "mx6sl-ntx" ]; then
        usleep 400000
    fi
    /etc/init.d/on-animator.sh
) &

if lsmod | grep -q sdio_wifi_pwr; then
    killall restore-wifi-async.sh enable-wifi.sh obtain-ip.sh udhcpc default.script wpa_supplicant 2>/dev/null
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
