# HB-RC-12-EP(-C/-BW)

<img width=400 src="https://raw.githubusercontent.com/jp112sdl/HB-RC-12-EP/master/Images/Bild.jpeg"></img>

## Parts
- ATMega **1284P** (für den [Prototypen](https://raw.githubusercontent.com/jp112sdl/HB-RC-12-EP/master/Images/Bild%202.jpeg) habe ich die [HM-ES-PWMSw1-Pl_GosundSP1-Platine](https://raw.githubusercontent.com/stan23/HM-ES-PMSw1-Pl_GosundSP1/master/Bilder/Platine_V2_bestückt.jpg) missbraucht ^^)
- 13 Taster (12 Tasterkanäle + 1 Anlerntaster)
- 2 Widerstände 330 Ohm
- 2 LEDs oder 1 Dual-Color-LED
- 2.9" ePaper Display
  - 3 Color [(schwarz/weiß/gelb)](https://www.exp-tech.de/displays/e-paper-e-ink/8516/296x128-2.9-e-ink-yellow/black/white-3-color-display-module)
  - 3 Color [(schwarz/weiß/rot)](https://www.exp-tech.de/new/8288/296x128-2.9-e-ink-display-module-three-color)
  - Monochrom [(schwarz/weiß)](https://www.exp-tech.de/displays/e-paper-e-ink/8324/2.9-e-paper-display-modul-mit-spi-interface?c=1424)
## Arduino
- [MCUDude/MightyCore Board](https://github.com/MCUdude/MightyCore)-Unterstützung für den 1284P
  - Pinout: Standard
  - Clock: 8MHz internal
  - BOD: Disabled
  - Compiler LTO: Disabled
  - Variant: 1284P
- Bibliotheken:  
  - [AskSinPP](https://github.com/pa-pa/AskSinPP) (master-Branch verwenden!)
  - [Low-Power](https://github.com/rocketscream/Low-Power)
  - [EnableInterrupt](https://github.com/GreyGnome/EnableInterrupt)
  - [GxEPD](https://github.com/ZinggJM/GxEPD) 
  - [Adafruit-GFX](https://github.com/adafruit/Adafruit-GFX-Library)
  - [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX)
  
 ## Forum
 Thread zum Projekt im Homematic Forum:</br>
 https://homematic-forum.de/forum/viewtopic.php?f=76&t=50160&p=503115#p503115

 ## PCB 
 TomMajor stellt ein passendes PCB zur Verfügung</br>
 https://github.com/TomMajor/SmartHome/tree/master/PCB/HB-RC-12-EP
 
 ## mögliche Probleme im Betrieb
 - Nach jeder Übertragung der Konfgurationsparameter, wird im Anschluss das Display aktualisiert.
  Dies dauert ein paar Sekunden. Auch beim Löschen von Direktverbindungen, was man mit "Config" bestätigen muss. In dieser Zeit reagiert die Fernbedienung auf keine weiteren Tasteneingaben.
 - Bei Direktverknüpfungen mit Geräten, die über BURST kommunizieren, bspw. Batterieaktoren und Heizkörperthermostate, kann es bei "longpress" u.U. auftreten, dass die Fernbedienung zyklisch weiter sendet. Hier muss in den Geräteeinstellungen am jeweiligen Kanal die "Mindestdauer für langen Tastendruck" erhöht werden.</br>1.8 Sekunden ist ein guter Startwert, den man iterativ nach unten korrigieren kann.
 


