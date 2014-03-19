rtap-ko
=======

Wireless Packet forwarder with filtering capabilities

On a Linux platform that supports WiFi monitor mode, this kernel module
listens on the monitor interface (ex: iw interface add mon0 type monitor)
for wireless frames that are prepended with a RadioTap header. It then
can forward those frames on to a listener on a remote (or local) server.
