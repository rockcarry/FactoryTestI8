#/bin/sh

if [ -f /mnt/sdcard/sharetronic_check/forcecopy.kpa ]; then
    rm -rf /etc/apkft
fi

if [ -f /etc/apkft/stage ]; then
    echo "apkft exist..."
else
    echo "copy apkft..."
    mkdir /etc/apkft
    cp ./appMicSn /etc/apkft/appApkft
    cp ./startrecord.pcm /etc/apkft/startrecord.pcm
    echo "all" > /etc/apkft/stage
    cp ./apkft.sh /etc/apkft/apkft.sh
    chmod 777 /etc/apkft/apkft.sh

    if [ -f /etc/init.d/rcS.dh ]; then
        echo "rcS.dh exist!!!"
    else
        echo "backup rcS.dh"
        mv /etc/init.d/rcS /etc/init.d/rcS.dh
    fi
    cp ./rcS /etc/init.d/rcS
    cp ./wifi_efuse_8189fs.map /etc/apkft/wifi_efuse_8189fs.map
    cp ./config.uvc /etc/apkft/config.uvc
    cp ./usbcamera_720av /etc/apkft/usbcamera_720av
fi
