phytool
=======

Linux MII register access


Usage
-----

    phytool read  iface/phy/reg
	phytool write iface/phy/reg val
    phytool print iface/phy[/reg]
    
    The PHY argument is either in clause-22 direct adressing syntax, or in
    clause-45 syntax `port:dev`. Where `port` is the MDIO port address and
    `dev` is the device's PHY address (0x1-0xA), MAC register (0x10-0x1A),
    the global register (0x1B), or the global2 register (0x1C)

    Some devices have a SERDES specific page on `dev` address 0xF


Examples
--------

    ~ # phytool read eth4/0/4
    0x0de1

    ~ # phytool print eth4/0
    ieee-phy: id:0x01410eb1
    
       ieee-phy: reg:BMCR(0x00) val:0x0800
          flags: reset loopback aneg-enable power-down isolate aneg-restart collision-test
          speed: 10-half
          ○ ○ ○ ○  ◉ ○ ○ ○  ○ ○ ○ ○  ○ ○ ○ ○
             0        8        0        0
    
       ieee-phy: reg:BMSR(0x01) val:0x7949
          link capabilities: 100-b4 100-f 100-h 10-f 10-h 100-t2-f 100-t2-h
          flags: ext-status aneg-complete remote-fault aneg-capable link jabber ext-register
          ○ ◉ ◉ ◉  ◉ ○ ○ ◉  ○ ◉ ○ ○  ◉ ○ ○ ◉
             7        9        4        9


Origin & References
-------------------

phytool is developed and maintained by Tobias Waldekranz.  
Please file bug fixes and pull requests at [GitHub][]

[GitHub]: https://github.com/wkz/phytool
