# SCan

Yet another CAN Hacker implementation based on _Sigma 10_ hardware.

Supports:
- 2 CAN channels
- native USB (without packet loss)
- 'russian can-hacker' protocol (extended compared with original lawicel)


# TODO
add connector schematics

# Building

Use QtCreator with arm-none-eabi-gcc compiler.
Can be flashed through STM's DFU utilities.

To start, you need to flash the DFU loader via st-link/j-ling etc.

# PC side
Now you can use http://canhacker.ru/ utility.
A more advanced shell is planned for further development.
