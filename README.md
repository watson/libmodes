# libadsb

[![Build status](https://travis-ci.org/watson/libadsb.svg?branch=master)](https://travis-ci.org/watson/libadsb)

This is a C library for decoding Mode S messages from aviation
aircrafts. It supports both standard Mode S Acquisition Squitter
messages (56 bits) and Mode S Extended Squitter messages (112 bits) that
also carry ADS-B information.

This project is a refactoring of the popular
[dump1090](https://github.com/antirez/dump1090) project by Salvatore
Sanfilippo. It modularizes the code into separate functions and removes
all non-essentials, so that only the decoding logic is left.

This project is still very much WIP.

## License

BSD-2-Clause
