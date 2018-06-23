iCE Floorplan
=============

iCE Floorplan is a floorplan viewer tool written for [Project IceStorm][icestorm], but intended to support any desired FPGA family. It renders logic and routing (TODO: routing. lol.) in a way reminiscent of electronics schematics.

Building
--------

This tool should work on any OS that has [Qt 5][qt], although the author only uses it on Linux. It needs the core, gui and widgets Qt libraries; on Debian-based systems, these can be installed with:

```sh
sudo apt-get install qtbase5-dev qtbase5-dev-tools
```

Once you have the dependencies, build the project with:

```sh
mkdir build
cd build
qmake ..
make
```

The `icefloorplan` (`icefloorplan.exe`, `icefloorplan.app`) binary is ready to be used.

Using
-----

An example bitstream (blinky on iCE40-LP384) can be opened with `File`→`Open Example`. An arbitrary bitstream can be opened with `File`→`Open...`.

The floorplan can be navigated either using mouse or touchpad (zoom with Ctrl+wheel), or using a touchscreen.

License
-------

[0-clause BSD](LICNSE-0BSD.txt)

[icestorm]:
[qt]:
