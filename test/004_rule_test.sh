 sudo dmesg -c && sudo modprobe -r rtap && make clean && make && sudo make install && sudo modprobe rtap && dmesg
 cat /proc/rtap/listeners 
 echo "1 127.0.0.1 8000" | sudo tee /proc/rtap/listeners 
 dmesg 
 echo "2 127.0.0.1 6000" | sudo tee /proc/rtap/listeners 
 dmesg 
 cat /proc/rtap/listeners 
 echo "1 2 1" | sudo tee /proc/rtap/rules 
 dmesg 
 echo "2 2 2" | sudo tee /proc/rtap/rules 
 dmesg 
 cat /proc/rtap/rules
 grep "" /proc/rtap/*
