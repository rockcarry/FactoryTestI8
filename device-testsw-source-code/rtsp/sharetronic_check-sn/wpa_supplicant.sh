#!/bin/sh

sleep 1
ifconfig wlan0 up
sleep 1
wpa_supplicant -D wext -i wlan0 -c  /etc/conf/wpa.conf -B > /dev/null 2>&1 &
sleep 1
WLANNAME=`cat /etc/wlanname`
if [ "$WLANNAME" == "wlan0" ]; then
    echo "wifi ok"
else
    echo "wifi ng"
fi
udhcpc -i wlan0 -p /var/run/udhcpc.pid &

# blue led on
echo 49 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio49/direction
echo 1 > /sys/class/gpio/gpio49/value

echo "turn on wifi..."
/opt/network/wifi_cmd.sh connect I8Iperf "I8Iperf888" WPA2 &

# copy factory test files
echo "+copy factory test files"
./copyapkft.sh
echo "-copy factory test files"

iperf3 -s &
./appMicSn iperf &

