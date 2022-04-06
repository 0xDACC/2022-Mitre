# 2022 Mitre eCTF: 0xDACC Edition!
This repo contains 0xDACC's code for this years challenge (the SAFFIRe bootloader)! It meets all functional and security requirements, and even catches edge cases! Try and attack our design, we dare you! 

We have included a [Getting Started Guide](getting_started.md) similar to the example which is below

There is also a detailed [reference guide](Reference_Document.md) to explain in details the functionality of the device

There are also detailed readme documents for the [bootloader](bootloader/README.md) and the [host tools](host_tools/README.md)

## Getting Started
Please see the [Getting Started Guide](getting_started.md).

## Project Structure
The code is structured as follows

* `bootloader/` - Contains everything to build the SAFFIRE bootloader. See [Bootloader README](bootloader/README.md).
* `configuration/` - Directory to hold raw and protected configuration images. The repo comes with an example unprotected configuration binary.
* `dockerfiles/` - Contains all Dockerfiles to build system.
* `firmware/` - Directory to contain raw and protected firmware images. The repo comes with an example unprotected firmware binary.
* `host-tools/` - Contains the host tools.
* `platform/` - Contains everything to run the avionic device.
* `tools/` - Miscellaneous tools to run and interract with SAFFIRe.
* `saffire.cfg` - An example option config file for running SAFFIRe

