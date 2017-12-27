# PIC
Small Programs for the PIC Processor.

Notes:
- Compiled using Small Device C Compiler using snapshot build version sdcc-snapshot-amd64-unknown-linux2.5-20171224-10186.tar.bz2
- In addition to installing SDCC, install the following packages:  libc6-pic and gputils.
- Build .hex file using build.sh.

Processors:
- Initially, I tried to get this to work for the 10f322.  Was able to generate the .asm code, but the linker failed everytime.  It appears to be a problem in pic10f322.lib, so I stopped.  You can test which processors it works for using the gputils/lib, there is a test build script.  It tries to compile against every processor in the list.  Note:  The 10f322 failed in this test.  The 16f690 was ok, so moved on with that one.

Programming:
- Programmed using the PicKit3 programming utility (Windows utility - See www.microchip.com/forums/m500342.aspx)



