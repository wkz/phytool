phytool
=======

Linux MDIO register access

Usage
-----

    phytool read  IFACE/ADDR/REG
    phytool write IFACE/ADDR/REG <0-0xffff>
    phytool print IFACE/ADDR[/REG]

    where

    ADDR := C22 | C45
    C22  := <0-0x1f>
    C45  := <0-0x1f>:<0-0x1f>
    REG  := <0-0x1f>

Note: Not all MDIO drivers support the `port:device` Clause 45 address
      format.

The `read` and `write` commands are simple register level
accessors. The `print` command will pretty-print a register. When
using the `print` command, the register is optional. If left out, the
most common registers will be shown.

Examples
--------

    ~ # phytool read eth4/0/4
    0x0de1

    ~ # phytool print eth0/0
    ieee-phy: id:0x01410eb1

       ieee-phy: reg:BMCR(0x00) val:0x1140
          flags:          -reset -loopback +aneg-enable -power-down -isolate -aneg-restart -collision-test
          speed:          1000-full

       ieee-phy: reg:BMSR(0x01) val:0x7949
          capabilities:   -100-b4 +100-f +100-h +10-f +10-h -100-t2-f -100-t2-h
          flags:          +ext-status -aneg-complete -remote-fault +aneg-capable -link -jabber +ext-register


mv6tool
=======

Marvell Link Street register access

Usage
-----

    mv6tool read  LOCATION/REG
    mv6tool write LOCATION/REG <0-0xffff>
    mv6tool print LOCATION[/REG]
    mv6tool print IFACE

    where

    LOCATION := IFACE/<port|phy> | DEV/<ADDR|phyN|portN|globalG|serdes>

    DEV  := <0-0x1f>
    ADDR := <0-0x1f>
    N    := <0-0xa>
    G    := <0-2>
    REG  := <0-0x1f>

The `read` and `write` commands are simple register level
accessors. The `print` command will pretty-print a register. When
using the `print` command, the register is optional. If left out, the
most common registers will be shown.

Examples
--------

    ~ # mv6tool 1/global1/0
    mv6: model:mv88e6097 dev:1 global:1
       mv6: reg:00 val:0xc800

    ~ # mv6tool print eth1-1
    mv6: model:mv88e6352 dev:0 port:1
       mv6: reg:PS(0x00) val:0x100f
          flags:          -pause-en -my-pause +phy-detect -link -eee -tx-paused -flow-ctrl
          speed:          10-half
          mode:           0xf

       mv6: reg:PC(0x04) val:0x1414
          flags:          -router-header +igmp-snoop -vlan-tunnel -tag-if-both
          egress-mode:    01, untagged
          frame-mode:     00, normal
          initial-pri:    01, tag prio
          egress-floods:  01, allow UC
          port-state:     00, disabled

Origin & References
-------------------

phytool is developed and maintained by Tobias Waldekranz.  
Please file bug fixes and pull requests at [GitHub][]

[GitHub]: https://github.com/wkz/phytool
