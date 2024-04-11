#!/bin/bash
sudo iptables -A OUTPUT -p tcp --dport 3000 -j DROP
sudo iptables -A OUTPUT -p tcp --dport 3002 -j DROP
sudo iptables -A OUTPUT -p tcp --dport 4545 -j DROP
sudo iptables -A OUTPUT -p tcp --dport 5001 -j DROP