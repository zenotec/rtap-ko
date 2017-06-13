sudo dmesg -c
sudo modprobe -r rtap
make clean
make
sudo make install
sudo modprobe rtap
dmesg

echo "mon0" | sudo tee /proc/rtap/devices
dmesg 
cat /proc/rtap/devices

echo "-mon0" | sudo tee /proc/rtap/devices
dmesg 
cat /proc/rtap/devices

grep "" /proc/rtap/*


