# dbh7a3

Porting db48x on a NUCLEO-H7A3ZI-Q, main goals : testing low power, a new keyboard layout and a 3.2'' display

The stm32h743 has no low power mode and no smps regulator

I still use a h7 and not a u5 for one reason : documentations and demos are more complete for h7.


Measuring current on JP4, I can achieve a stop mode 2 at 30µA and 13mA running at 120Mhz, full speed 280Mhz when the usb is connected

A Sharp lcd is still used, but in 3.2'' with a 536x336 resolution 

A µsd card is used for storage

The keyboard is horizontal : 3x6 keys under the lcd and 1x6 + 5x5 keys at the right side, the layout is an attempt

<img width="536" height="336" alt="image_hr" src="https://github.com/user-attachments/assets/e5dbc27c-fdae-4213-8231-74d21b85713f" />

__weak void DBx_Task_App(void) in DBx_main.c is used to test the lcd, the keyboard, the usb task and the file system.


<img width="1539" height="760" alt="image" src="https://github.com/user-attachments/assets/acef5a0a-a5b5-46da-9707-fae9479d1002" />

DB48x is running.
<img width="536" height="336" alt="demo3 2" src="https://github.com/user-attachments/assets/d911e336-1f95-4314-81e8-c1900c6d34e9" />

This keyboard layout is a work in progress. But the new resolution is very easy to adapt !

Some bugs in display, (no refresh) during some tests.
The 4 edit keys are working, transalpha mode is ok, only some planes are correct, help function is working.



