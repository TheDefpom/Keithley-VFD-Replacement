This project allows you to replace an aged Keithley 2002 VFD with a 4.58" colour TFT, (and in theory a 2001 which I will be testing this shortly).

The project uses an off the shelf ESP32-S3 Qualia microsontroller board from Adafruit, along with a 4.58" TFT, get both from Adafruit, links are in the code header.

THe project is built using the Arduino IDE V2, and the Adafruit GFX Library, and board definition for the Qualia.

The tricky part is the font used, I had to modify a "good enough" version fo the main readout as it needed to match the char list for the ohms, mu, and degrees symbols as used in the 5x7 default font from the GFX library, if you find (or make) a better font for this let me know so it can be included. however the 5x7 works very well for the sub readout (the bottom display section) as it has chars that work perfectly for the graphing it has.

I have made a video showing the reverse engineering, proof of conecpts, and the final build and installation of the TFT into my Keithley 2002, this can be seen here: https://youtu.be/HHad28dQnug

Original credit and links must remain in the code header for any forks of the code.



MIT License

Copyright (c) [year] [fullname]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
