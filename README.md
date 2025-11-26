# FM-radio-receiver-with-digital-tuning

## Team members
* Mezera Vojtěch
* Moravec David
* Pavlíček Michal
* Mostecký Filip

## Abstract
The main objective of this project is to implement fully functional FM radio receiver with digital tuning on ATmega328P-based Arduino Uno board.
The tuner module Si4703 is controlled over the I<sup>2</sup>C bus. I<sup>2</sup>C OLED display is used for the main user interface.
Basic push buttons are used for controlling the receiver. This project also features the RDS (Radio Data System) to show the station name and radio text.  

## Used components
* Arduino UNO
* FM radio tuner module Si4703 with RDS
  Receives FM broadcast band, approx. 87.5 - 108 MHz
  Provides support for RDS
* OLED display 128x64 1.3" I<sup>2</sup>C  
  Main user interface
  Displays tuned frequency, station name and text it's text (if avaible)
  Also shows status messages such as scanning progress or number of found stations
