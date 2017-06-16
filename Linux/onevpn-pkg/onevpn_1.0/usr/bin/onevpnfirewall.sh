#!/bin/sh
#cleat the old crap
iptables -F
iptables -X
ip6tables -F
ip6tables -X

echo "<<< here"

#default policy
iptables -P INPUT DROP
iptables -P OUTPUT ACCEPT
iptables -P FORWARD DROP
ip6tables -P INPUT DROP
ip6tables -P OUTPUT DROP
ip6tables -P FORWARD DROP

#allow existing connections to continue
iptables -A input -m state --state RELATED,ESTABLISHED -j ACCEPT
ip6tables -A input -m state --state RELATED,ESTABLISHED -j ACCEPT

#allow packets from loopback interface
iptables -A INPUT -i lo -j ACCEPT
ip6tables -A INPUT -i lo -j ACCEPT 
