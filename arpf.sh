#!/bin/bash
./impersonate&
tcpdump -i eth0 | ./arpf.pl
kill $!
