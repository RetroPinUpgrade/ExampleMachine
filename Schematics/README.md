# Schematics for RPU Boards
Sources for parts can be found here: https://pinballindex.com/index.php/Part_Sources  

## Rev 4  
This board plugs into the J5 connector of AS-2518-17, AS-2518-35, MPU100, or MPU200 boards. It uses all through-hole compoents and has a socket for a WiFi module as well as an OLED display (see note below about OLED power). Using the HALT line, this board can allow/disallow the original code from running.

## Rev 101  
This board plugs into the CPU socket of any machine with a Motorola 6800, 6802, or 6808 processor. It has a socket for the original processor so the operator can choose to boot to original or new code. This board has 3 serial ports and one i2c port exposed for expansion. The diagnostics jumper is very close to the USB port of the Arduino, so use right angle header pins for the diagnostics jumper.

## Rev 102  
This board plugs into the CPU socket of any machine with a Motorola 6800, 6802, or 6808 processor. It has a socket for the original processor so the operator can choose to boot to original or new code. This board has a socket for a WiFi module as well as an OLED display (see note below about OLED power). By detecting the clock source, this board automatically operates with a 6800 or 6802/6808 without the need of changing jumpers.  

### How to Order RPU rev 102 Surface Mount  
1) go to JLCPCB.com  
2) click on PCB Assembly  
3) Upload Gerber File "RPU V102 Gerber"
4) Choose PCB Quantity
5) Default settings are okay - click on PCB Assembly
6) Click Confirm, Click Next
7) Add BOM file "RPU V102 BOM"
8) Add CPL file "RPU V102 Pick and Place"
9) On the Bill of Materials, the boxes for C1-C5, R1-R8, and U5-U8 should be checked already (you want all these)
10) Click Next until your order is done
  
## OLED Power  
Some 1.3" OLED displays come with pin 1=GND and 2=VCC, and others are exactly the opposite. For that reason, the VCC/GND supplies for the OLED header are controlled with jumpers. Solder or jumper pins 1-2 and pins 1-2 under the OLED display for 1=VCC and 2=GND. Solder or jumper pins 2-3 and pins 2-3 under the OLED display for 1=GND and 2=VCC.  
