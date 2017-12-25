# PIC
Small Programs for the PIC Processor.

Notes:
- Compiled using Small Device C Compiler using snapshot build version sdcc-snapshot-amd64-unknown-linux2.5-20171224-10186.tar.bz2
- Installed the following packages:
  - libc6-pic
  - gputils
- build using a simple build script, see build.sh for each project.
- Initially tried getting a project to compile using the 10f322, but always got a linker error.  Worked fine using the 16f690.

- Programmed using the PicKit3 programming utility (Windows utility - See www.microchip.com/forums/m500342.aspx)
- Set config bits in main.c or using the programming utility.



