# Project to drive a large vacuum fluorescent display

The display stems from an old Thermo Finnigan Mass Spectrometer (TSQ/SSQ 7000) my university wanted to dispose, so I rescued this cool display.
<div style="width:300px">
![TSQ/SSQ 7000](/images/GC-MS.jpg)
<div>


## Display 
Type: 03601-34-080 FLIP VFD
Manufacturer IEE
20 characters x 4 rows with a hight of 11.3 mm per character.
Due to an old version of the manufacturers website the interface is serial and software dimming is available.
Input voltage 5 V (12 Watt) -> 2.4 A
<div style="width:300px">
![03601-34-080 FLIP VFD Front](/images/Display.jpg)
<div>

### Datasheets
I found no datasheet for exactly this device, however I found some info around the internet:
[datasheet of 03601-30-040R](https://www.farnell.com/datasheets/1681172.pdf) - functionally closest version
["Backup"](https://datasheet.octopart.com/3601-26-240-IEE-datasheet-46887.pdf) of the manufacturers website

### Pinout & Jumper
<div style="width:300px">
![03601-34-080 FLIP VFD Front](/images/Display.jpg)
<div>

#### RS-232 - 25 pin connector
[x] - connections present in the GC-MS
```
1  - [ ] Chassis GND\
2  - [x] TX (RS-232C)           
3  - [x] RX (RS-232C)
5  - [ ] Clear to Send (RS-232C)
7  - [x] Signal GND
9  - [ ] +10V out
10 - [ ] -10V out
11 - [ ] TX- (RS-422)
15 - [ ] RX- (RS-422)
17 - [ ] RX+ (RS-422)
18 - [ ] TX+ (RS-422)
20 - [ ] Data Terminal Ready (RS-232C)
```
Al other pins are not connected!

#### power connector - 6 pin connector
According to the datasheet of other serial VFDs of the same manufacturer and of the same time, the inputs are ttl compatible. However, the functionally closest version did not mention this.

Power cable had 3 connected pins in the original device
```
1  - +5V
4  - GND
6  - RESET (Pull low to reset)
```
Al other pins are not connected!

#### Jumper settings 
The settings were extrapolated from a datasheet of a S03601-30-040R display
```
E5 -> E10 Burn-In-Test      (no reaction from my display)
E4 -> E9  Self-Test         (works, every character is displayed)
E3 -> E8  Even Parity
E2 -> E7  Disable Parity - 2 Stop Bits
E1 -> E6  From 9600 to 1200 baud
```
  
### Character set
The display is using ASCII and has has a few alternate character sets which can be selected as shown in the datasheet.


<div style="width:300px">
![Closeup & Self-Test](/images/Testmode.jpg)
<div>
