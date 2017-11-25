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

## Usage

```c
#include "decoder.h"

void on_msg(mode_s_t *self, struct mode_s_msg *mm) {
  printf("Got message from flight %s at altitude %d\n", mm->flight, mm->altitude);
}

int main(int argc, char **argv) {
  mode_s_t state;
  uint32_t data_len;
  unsigned char *data;
  uint16_t *mag;

  data_len = 262620;        // size of data buffer
  data = malloc(data_len);  // holds raw IQ samples from an antenna (2 char per sample)
  mag = malloc(sizeof(uint16_t)*(data_len/2)); // holds amplitude of the signal

  // initialize the decoder state
  mode_s_init(&state);

  // get some raw IQ data somehow
  get_samples(data);

  // compute the magnitude of the signal
  mode_s_compute_magnitude_vector(data, mag, data_len);

  // detect Mode S messages in the signal and call on_msg with each message
  mode_s_detect(&state, mag, data_len/2, on_msg);
}
```

Check out
[`tests/test.c`](https://github.com/watson/libadsb/blob/master/tests/test.c)
for a complete example.

## License

BSD-2-Clause
