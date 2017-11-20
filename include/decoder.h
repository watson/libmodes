#ifndef __ADSB_DECODER_H
#define __ADSB_DECODER_H

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

#define MODE_S_ICAO_CACHE_LEN 1024 // Power of two required
#define MODE_S_LONG_MSG_BYTES (112/8)

// Program state
typedef struct {
  // Internal state
  uint32_t icao_cache[sizeof(uint32_t)*MODE_S_ICAO_CACHE_LEN*2]; // Recently seen ICAO addresses cache

  // Configuration
  int fix_errors; // Single bit error correction if true
  int aggressive; // Aggressive detection algorithm
  int check_crc;  // Only display messages with good CRC
} adsb_mode_s_t;

// The struct we use to store information about a decoded message
struct adsb_mode_s_msg {
  // Generic fields
  unsigned char msg[MODE_S_LONG_MSG_BYTES]; // Binary message
  int msgbits;                // Number of bits in message
  int msgtype;                // Downlink format #
  int crcok;                  // True if CRC was valid
  uint32_t crc;               // Message CRC
  int errorbit;               // Bit corrected. -1 if no bit corrected.
  int aa1, aa2, aa3;          // ICAO Address bytes 1 2 and 3
  int phase_corrected;        // True if phase correction was applied.

  // DF 11
  int ca;                     // Responder capabilities.

  // DF 17
  int metype;                 // Extended squitter message type.
  int mesub;                  // Extended squitter message subtype.
  int heading_is_valid;
  int heading;
  int aircraft_type;
  int fflag;                  // 1 = Odd, 0 = Even CPR message.
  int tflag;                  // UTC synchronized?
  int raw_latitude;           // Non decoded latitude
  int raw_longitude;          // Non decoded longitude
  char flight[9];             // 8 chars flight number.
  int ew_dir;                 // 0 = East, 1 = West.
  int ew_velocity;            // E/W velocity.
  int ns_dir;                 // 0 = North, 1 = South.
  int ns_velocity;            // N/S velocity.
  int vert_rate_source;       // Vertical rate source.
  int vert_rate_sign;         // Vertical rate sign.
  int vert_rate;              // Vertical rate.
  int velocity;               // Computed from EW and NS velocity.

  // DF4, DF5, DF20, DF21
  int fs;                     // Flight status for DF4,5,20,21
  int dr;                     // Request extraction of downlink request.
  int um;                     // Request extraction of downlink request.
  int identity;               // 13 bits identity (Squawk).

  // Fields used by multiple message types.
  int altitude, unit;
};

typedef void (*adsb_cb_t)(adsb_mode_s_t *self, struct adsb_mode_s_msg *mm);

void adsb_init(adsb_mode_s_t *self);
void adsb_compute_magnitude_vector(unsigned char *data, uint16_t *mag, uint32_t size);
void adsb_detect_mode_s(adsb_mode_s_t *self, uint16_t *mag, uint32_t maglen, adsb_cb_t);

#endif
