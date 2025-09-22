# dbh7a3
Porting db48x on a NUCLEO-H7A3ZI-Q
The stm32h743 has no low power mode and no smps 1.8V 

I use still a h7 and not a u5 for one reason : documentations and demos are more complete for h7

Measuring current on JP4, I can achieve a stop mode 2 at 30µA and 13mA running at 120Mhz, full speed 280Mhz when the usb is connected
A Sharp lcd is still used, but in 3.2'' with a 536x336 resolution 
A µsd card is used for storage
The keyboard is horizontal : 3x6 keys under the lcd and 1*6 + 5*5 keys at the right side
The layout is an attempt

