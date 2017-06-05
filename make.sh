 sudo modprobe -r rtap
 sudo dmesg -c
 make clean && make && sudo make install
 sudo modprobe rtap
 echo "wlan-5_8.2" | sudo tee /proc/rtap/devices 
 dmesg 
 echo "127.0.0.1 8800" | sudo tee /proc/rtap/listeners
 dmesg 
 echo "1 3 4 2 0080" | sudo tee /proc/rtap/filters 
 dmesg 
 grep "" /proc/rtap/*
