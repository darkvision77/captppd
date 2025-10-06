# captppd
[![Core tests](https://github.com/darkvision77/captppd/actions/workflows/core-tests.yml/badge.svg)](https://github.com/darkvision77/captppd/actions/workflows/core-tests.yml)

CUPS driver for Canon CAPT v1 printers based on reverse engineering of the original driver. \
Implemented as a CUPS backend using libusb.

The driver is divided into two parts: the CUPS side (this repository)
and the protocol side ([libcapt](https://github.com/darkvision77/libcapt)).

> [!IMPORTANT]
> This project is currently in an EXPERIMENTAL state.

## Target devices
| Model   | Cartridge     | PPM (A4) | Max Resolution | Year (approx.) |
|---------|---------------|----------|----------------|----------------|
| LBP800  | EP-22         | 8        | 600 dpi        | 1999-2001      |
| LBP810  | EP-22         | 8        | 600 dpi        | 2001-2002      |
| LBP1120 | EP-22         | 10       | 600 dpi        | 2003-2004      |
| LBP1210 | EP-25         | 14       | 600 dpi        | ~2002          |
| LBP3200 | EP-26/EP-27   | 18       | 600 dpi        | 2004-2006      |

## Status
It cannot be said that the driver is stable until there are appropriate tests on the verified simulator.

At the moment, the protocol is quite well known.
The raster format (SCoA compression algorithm) is also well known and,
theoretically, is completely reliable.

## Compilation
### Dependencies
#### Compile-time
- gcc >= 11 or clang >= 16
- cmake >= 3.21
- git
- [libcapt](https://github.com/darkvision77/libcapt) (downloaded automatically by CMake FetchContent)

#### Run-time
- cups
- libusb-1.0

### Compile
```sh
cmake -S. -B build
cmake --build build
```

### Install
This command will install the backend, PPD files, and quirks
that prevent Canon CAPT v1 printers from being detected by the CUPS USB backend:
```sh
cmake --install build
```

If you don't want to blacklist Canon CAPT v1 printers for the CUPS USB backend, you can use:
```sh
cmake --install build --component=base
```

## Usage
### Adding printer from command line
1. List devices:
```sh
lpinfo -v

# Example output
direct usb://Canon/LBP3200?serial=00000000
direct captusb://Canon/LBP3200?drv=capt&serial=00000000
```

2. Copy captusb URI and add printer using lpadmin:
```sh
lpadmin -p LBP3200 -E -v 'captusb://Canon/LBP3200?drv=capt&serial=00000000' -m LBP3200CAPTPPD.ppd
```

3. Verify with lpstat:
```sh
lpstat -v

# Example output
device for LBP3200: captusb://Canon/LBP3200?drv=capt&serial=00000000
```

### Adding printer from user interface
#### CUPS web interface
1. Go to admin page (default: http://127.0.0.1:631/admin).
2. Click «Find New Printers».
3. Select the captusb printer.
4. And the model should be like `Canon LBP3200, captppd 0.1.0`.

## Troubleshooting
### If the printer has not been detected
1. Make sure that your printer is displayed in the `lsusb` output.
2. Execute the backend directly:
```sh
sudo $(cups-config --serverbin)/backend/captusb
```
3. If the backend couldn't detect the printer, [create an issue](https://github.com/darkvision77/captppd/issues/new).

### If the printer is not working
1. Check `lpstat -v` and make sure that the `captusb` backend is being used.
2. Make sure you are using the correct PPD (should be like `Canon LBP3200, captppd 0.1.0`, not `Canon LBP3200 CAPT ver.1.5`).
3. Enable CUPS debug logging:
```sh
cupsctl --debug-logging
```
4. See logs at `/var/log/cups/error_log`. \
To filter out unwanted messages, you can use grep:
```sh
grep '\] \[Job' /var/log/cups/error_log
```
5. [Create an issue](https://github.com/darkvision77/captppd/issues/new) and attach the log.

## See also
- [UoWPrint](https://printserver.ink/) — convert your old USB printer (or MFP) into Wi-Fi printer/MFP
- [mounaiban/captdriver](https://github.com/mounaiban/captdriver) — open source CUPS driver for the newer Canon LBP models

## License
captppd is licensed under a 2-clause BSD license.
