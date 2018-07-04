#!/bin/sh

# blue led on
echo 49 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio49/direction
echo 1 > /sys/class/gpio/gpio49/value

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
./led &

