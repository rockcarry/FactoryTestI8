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

# write wifisoc config
cp ./wifi_efuse_8189fs.map /tmp/wifi_efuse_8189fs.map
iwpriv wlan0 efuse_file /tmp/wifi_efuse_8189fs.map
iwpriv wlan0 efuse_set wlfk2map

echo "turn on wifi..."
/opt/network/wifi_cmd.sh connect I8SMT "I8SMT888" WPA2 &

echo "turn on usb rndis..."
cd /lib/modules/
insmod libcomposite.ko
insmod u_ether.ko
insmod usb_f_ecm.ko
insmod usb_f_ecm_subset.ko
insmod usb_f_rndis.ko
insmod g_ether.ko
ifconfig usb0 192.168.1.111 up
cd -

./carrier-server --st=jxh62 &
./appMicSn 192.168.1.100 &
