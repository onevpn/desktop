#!/bin/sh
#
# openvpn-tap-up-down.sh
#
#
# A script to be used as an OpenVPN bridged (tap) up/down script on Mac OSX 10.4
# - uses ipconfig to acquire a DHCP lease via the OpenVPN tap interface, and scutil to
#  incorporate the DHCP-supplied DNS configuration
#
# Use in your OpenVPN config file as follows:
#
#    up  openvpn-tap-up-down.sh
#
# - up: openvpn calls the 'up' script after the tun/tap interface is created, but before the link
#   to the server is available for use (ditto 'up-delay' at least for UDP)
#   - on testing w/ openvpn 2.0.5, and tcpdump on the tap interface as soon as it comes up,
#     packets are queued up on the interface (and not actually sent over the openvpn tunnel)
#     until *after* this script returns; this makes sense: this script could fail in which
#     case the connection is invalid
#     - this means the DHCP acquisition can't complete until after this script exits
#     - that's not directly a problem as the OS X DHCP client should do everything we need
#       to make the interface functional, all by itself - *except* for one small thing: as of
#       OS X 10.4.7 the DHCP-acquired DNS information is not "merged" into the System
#       Configuration (OS X bug?)
#       - thus we have a chicken-and-egg situation: we need to manually fixup the DNS config,
#         but can't until we get the DHCP lease; we won't get the lease until we this script exits
#       - the solution is to spawn a little "helper" that waits until the lease is acquired,
#         and then does the DNS fixup
#
# - down: the only sensible 'down' action is to release the DHCP lease (as a courtesy to the
#   DHCP server), alas it's too late to do this *after* the connection has been shutdown (as
#   of OpenVPN 2.0 there's no "pre-disconnect" script option; note that both 'down' and
#   'down-pre' are called only after the connection to the server is closed ('down-pre' before
#   closing the tun/tap device, 'down' after)
#   - OS X automatically cleans up the System Config keys created from ipconfig, but we need to
#     manually remove the DNS fixup
#
# 2006-09-21    Ben Low    original
#
# 200x-xx-xx    name
#

if [ -z "$dev" ]; then echo "$0: \$dev not defined, exiting"; exit 1; fi

# relevant script_type values are 'up' or 'down'
case "$script_type" in
   up)

     # bring the interface up and set it to DHCP
     # - System Configuration dynamic store will be automatically updated, with the
     #       State:/Network/Service/DHCP-tap0
     #   data store created.
     # - the ipconfig man page notes that it should only be used for "test and debug" purposes,
     #   and that you're supposed to use the SystemConfiguration APIs to manipulate the network
     #   configuration
     #   - alas, there appears to be no CLI utility other than ipconfig

     /usr/sbin/ipconfig set "$dev" DHCP

     # spawn our little DNS-fixerupper
     {
         # whilst ipconfig will have created the neccessary Network Service keys, the DNS
         # settings won't actually be used by OS X unless the SupplementalMatchDomains key
         # is added
         # ref. <http://lists.apple.com/archives/Macnetworkprog/2005/Jun/msg00011.html>
         # - is there a way to extract the domains from the SC dictionary and re-insert
         #   as SupplementalMatchDomains? i.e. not requiring the ipconfig domain_name call?

         # - wait until we get a lease before extracting the DNS domain name and merging into SC
         # - despite it's name, ipconfig waitall doesn't (but maybe one day it will :-)
         /usr/sbin/ipconfig waitall

         # usually takes at least a few seconds to get a DHCP lease
         sleep 3
         n=0
         while [ -z "$domain_name" -a $n -lt 5 ]
         do
             sleep $n
             n=`expr $n + 1`
             domain_name=`/usr/sbin/ipconfig getoption $dev domain_name 2>/dev/null`
         done

         if [ "$domain_name" ]; then
         /usr/sbin/scutil <<EOF
d.init
get State:/Network/Service/DHCP-$dev/DNS
d.add SupplementalMatchDomains * $domain_name
set State:/Network/Service/DHCP-$dev/DNS
EOF
         fi

     } &

   ;;

   down)

     # for completeness...
     if [ `/usr/bin/id -u` -eq 0 ]; then
         /usr/sbin/ipconfig set "$dev" NONE
     fi

   ;;
   *) echo "$0: invalid script_type" && exit 1 ;;
esac

##### FIN


