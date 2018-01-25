# Sane Backend for the Mustek ScanExpress 12000p

This repository contains the source code for an experimental [Sane](http://www.sane-project.org/) backend.

The Scanner contains a 600dpi Color CCD with a 12bit color resolution per channel.
The Scanner was manufactured with different ASIC revisions and ADCs. This driver currently supports only the asic revision 0xa2 with the 12bit WM8144.

Things that work:

- Read/Write access to all ASIC registers
- Using the "DMA" transfer to read the 128kbyte internal FiFo
- Controlling the stepper motor for 50, 100,200,300 and 600dpi scans
- Controling the Lamp
- Calibrating the offset and gain prior to a scan and uploading the values to the WM8144
- Scanning gray images

Things that need work:

- Scanning color images (should not be to hard as reading the individual channels already works)
- Enable the ASIC internal image processing capabilities, currently all scans are done with the internal image processing disabled and the required things are done in SW.
- Getting EPP to work for faster data transfer between scanner and PC

## Code Organization

- a4s2600.cpp - The implementation of the ASIC register access
- parallelport.cpp - Helper class for accessing the parallel port under linux
- sane-backed.cpp - As the name suggests this is the implementation of the sane API
- sanedevicehandle.cpp - Class that bridges between the SANE world and the driver
- scannercontrol.cpp - Handling of the scanning and calibration processes
- wm8144.cpp - Implementation of the WM 8144 Registers

## Known Bugs and limitations

- If the scanning applications crashes before the scanner is in the CPU mode again, access to the scanner will fail until it is power-cycled
- Sometimes the scanner only returns a black image (May be a race-condition where the driver is not waiting long enougth for a setting to be applied?). Workaround: Scan again :)

