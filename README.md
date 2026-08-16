This project allows you to replace an aged Keithley 2002 VFD with a 4.58" colour TFT, (it will also work for the 2001).

The project uses an off the shelf ESP32-S3 Qualia microcontroller board from Adafruit, along with a 4.58" TFT, get both from Adafruit, links are in the code header.

The project is built using the Arduino IDE V2, using the Adafruit GFX Library, and the board definition for the Qualia.

The tricky part is the font used, I had to modify a "good enough" version fo the main readout as it needed to match the char list for the ohms, mu, and degrees symbols as used in the 5x7 default font from the GFX library, if you find (or make) a better font for this let me know so it can be included. however the 5x7 works very well for the sub readout (the bottom display section) as it has chars that work perfectly for the graphing which it has.

I have made a video showing the reverse engineering, proof of concepts, and the final build and installation of the TFT into my Keithley 2002, this can be seen here: https://youtu.be/HHad28dQnug

Original credit and links must remain in the code header for any forks of the code.


<img width="1709" height="1223" alt="thumbpic" src="https://github.com/user-attachments/assets/4f359531-740a-4ff7-80cc-f993cd2fa029" />



Revision history:
(Rev - Date - By - Changes)

Rev 1.0 - 17th July 2026 - The Defpom - Initial release

Rev 1.01 - 23rd July 2026 - The Defpom - Added space injection in main readout for 8.5 digit mode to improve readout ie: 1.23456789mV to 1.23456789 mV

Rev 1.02 - 26th July 2026 - The Defpom - Added display update timer to slow down display writing to sensible speed.
