#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include "decoder.h"

#define MODE_S_DATA_LEN (16*16384) // 256k
#define MODE_S_PREAMBLE_US 8       // microseconds
#define MODE_S_LONG_MSG_BITS 112
#define MODE_S_FULL_LEN (MODE_S_PREAMBLE_US+MODE_S_LONG_MSG_BITS)

#define MODE_S_NOTUSED(V) ((void) V)

pthread_t reader_thread;
pthread_mutex_t data_mutex; // Mutex to synchronize buffer access.
pthread_cond_t data_cond;   // Conditional variable associated.
unsigned char *data;        // Raw IQ samples buffer
int fd;                     // file descriptor.
uint32_t data_len;          // Buffer length.
int data_ready = 0;         // Data ready to be processed.
int should_exit = 0;        // Exit from the main loop when true.

// Capability table.
char *ca_str[8] = {
  "Level 1 (Survillance Only)",
  "Level 2 (DF0,4,5,11)",
  "Level 3 (DF0,4,5,11,20,21)",
  "Level 4 (DF0,4,5,11,20,21,24)",
  "Level 2+3+4 (DF0,4,5,11,20,21,24,code7 - is on ground)",
  "Level 2+3+4 (DF0,4,5,11,20,21,24,code7 - is on airborne)",
  "Level 2+3+4 (DF0,4,5,11,20,21,24,code7)",
  "Level 7 ???"
};

// Flight status table.
char *fs_str[8] = {
  "Normal, Airborne",
  "Normal, On the ground",
  "ALERT,  Airborne",
  "ALERT,  On the ground",
  "ALERT & Special Position Identification. Airborne or Ground",
  "Special Position Identification. Airborne or Ground",
  "Value 6 is not assigned",
  "Value 7 is not assigned"
};

void read_data_from_file(void) {
  pthread_mutex_lock(&data_mutex);
  while(1) {
    ssize_t nread, toread;
    unsigned char *p;

    if (data_ready) {
      pthread_cond_wait(&data_cond, &data_mutex);
      continue;
    }

		// Move the last part of the previous buffer, that was not processed, on
		// the start of the new buffer.
		memcpy(data, data+MODE_S_DATA_LEN, (MODE_S_FULL_LEN-1)*4);
    toread = MODE_S_DATA_LEN;
    p = data+(MODE_S_FULL_LEN-1)*4;
    while(toread) {
      nread = read(fd, p, toread);
      if (nread <= 0) {
        should_exit = 1; // Signal the other thread to exit.
        break;
      }
      p += nread;
      toread -= nread;
    }
    if (toread) {
			// Not enough data on file to fill the buffer? Pad with no signal.
			memset(p, 127, toread);
    }
    data_ready = 1;
    // Signal to the other thread that new data is ready
    pthread_cond_signal(&data_cond);
  }
}

// We read data using a thread, so the main thread only handles decoding
// without caring about data acquisition.
void *reader_thread_entry_point(void *arg) {
  MODE_S_NOTUSED(arg);
  read_data_from_file();
  return NULL;
}

char *get_me_description(int metype, int mesub) {
  char *mename = "Unknown";

  if (metype >= 1 && metype <= 4)
    mename = "Aircraft Identification and Category";
  else if (metype >= 5 && metype <= 8)
    mename = "Surface Position";
  else if (metype >= 9 && metype <= 18)
    mename = "Airborne Position (Baro Altitude)";
  else if (metype == 19 && mesub >=1 && mesub <= 4)
    mename = "Airborne Velocity";
  else if (metype >= 20 && metype <= 22)
    mename = "Airborne Position (GNSS Height)";
  else if (metype == 23 && mesub == 0)
    mename = "Test Message";
  else if (metype == 24 && mesub == 1)
    mename = "Surface System Status";
  else if (metype == 28 && mesub == 1)
    mename = "Extended Squitter Aircraft Status (Emergency)";
  else if (metype == 28 && mesub == 2)
    mename = "Extended Squitter Aircraft Status (1090ES TCAS RA)";
  else if (metype == 29 && (mesub == 0 || mesub == 1))
    mename = "Target State and Status Message";
  else if (metype == 31 && (mesub == 0 || mesub == 1))
    mename = "Aircraft Operational Status Message";
  return mename;
}

// This function gets a decoded Mode S Message and prints it on the screen in a
// human readable format.
void display_mode_s_msg(adsb_mode_s_t *self, struct adsb_mode_s_msg *mm) {
  int j;

  // Show the raw message.
  printf("*");
  for (j = 0; j < mm->msgbits/8; j++) printf("%02x", mm->msg[j]);
  printf(";\n");

  printf("CRC: %06x (%s)\n", (int)mm->crc, mm->crcok ? "ok" : "wrong");
  if (mm->errorbit != -1)
    printf("Single bit error fixed, bit %d\n", mm->errorbit);

  if (mm->msgtype == 0) {
    // DF 0
    printf("DF 0: Short Air-Air Surveillance.\n");
    printf("  Altitude       : %d %s\n", mm->altitude,
      (mm->unit == MODE_S_UNIT_METERS) ? "meters" : "feet");
    printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);
  } else if (mm->msgtype == 4 || mm->msgtype == 20) {
    printf("DF %d: %s, Altitude Reply.\n", mm->msgtype,
      (mm->msgtype == 4) ? "Surveillance" : "Comm-B");
    printf("  Flight Status  : %s\n", fs_str[mm->fs]);
    printf("  DR             : %d\n", mm->dr);
    printf("  UM             : %d\n", mm->um);
    printf("  Altitude       : %d %s\n", mm->altitude,
      (mm->unit == MODE_S_UNIT_METERS) ? "meters" : "feet");
    printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);

    if (mm->msgtype == 20) {
      // TODO: 56 bits DF20 MB additional field.
    }
  } else if (mm->msgtype == 5 || mm->msgtype == 21) {
    printf("DF %d: %s, Identity Reply.\n", mm->msgtype,
      (mm->msgtype == 5) ? "Surveillance" : "Comm-B");
    printf("  Flight Status  : %s\n", fs_str[mm->fs]);
    printf("  DR             : %d\n", mm->dr);
    printf("  UM             : %d\n", mm->um);
    printf("  Squawk         : %d\n", mm->identity);
    printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);

    if (mm->msgtype == 21) {
      // TODO: 56 bits DF21 MB additional field.
    }
  } else if (mm->msgtype == 11) {
    // DF 11
    printf("DF 11: All Call Reply.\n");
    printf("  Capability  : %s\n", ca_str[mm->ca]);
    printf("  ICAO Address: %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);
  } else if (mm->msgtype == 17) {
    // DF 17
    printf("DF 17: ADS-B message.\n");
    printf("  Capability     : %d (%s)\n", mm->ca, ca_str[mm->ca]);
    printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);
    printf("  Extended Squitter  Type: %d\n", mm->metype);
    printf("  Extended Squitter  Sub : %d\n", mm->mesub);
    printf("  Extended Squitter  Name: %s\n",
      get_me_description(mm->metype, mm->mesub));

    // Decode the extended squitter message.
    if (mm->metype >= 1 && mm->metype <= 4) {
      // Aircraft identification.
      char *ac_type_str[4] = {
        "Aircraft Type D",
        "Aircraft Type C",
        "Aircraft Type B",
        "Aircraft Type A"
      };

      printf("    Aircraft Type  : %s\n", ac_type_str[mm->aircraft_type]);
      printf("    Identification : %s\n", mm->flight);
    } else if (mm->metype >= 9 && mm->metype <= 18) {
      printf("    F flag   : %s\n", mm->fflag ? "odd" : "even");
      printf("    T flag   : %s\n", mm->tflag ? "UTC" : "non-UTC");
      printf("    Altitude : %d feet\n", mm->altitude);
      printf("    Latitude : %d (not decoded)\n", mm->raw_latitude);
      printf("    Longitude: %d (not decoded)\n", mm->raw_longitude);
    } else if (mm->metype == 19 && mm->mesub >= 1 && mm->mesub <= 4) {
      if (mm->mesub == 1 || mm->mesub == 2) {
        // Velocity
        printf("    EW direction      : %d\n", mm->ew_dir);
        printf("    EW velocity       : %d\n", mm->ew_velocity);
        printf("    NS direction      : %d\n", mm->ns_dir);
        printf("    NS velocity       : %d\n", mm->ns_velocity);
        printf("    Vertical rate src : %d\n", mm->vert_rate_source);
        printf("    Vertical rate sign: %d\n", mm->vert_rate_sign);
        printf("    Vertical rate     : %d\n", mm->vert_rate);
      } else if (mm->mesub == 3 || mm->mesub == 4) {
        printf("    Heading status: %d", mm->heading_is_valid);
        printf("    Heading: %d", mm->heading);
      }
    } else {
      printf("    Unrecognized ME type: %d subtype: %d\n", 
        mm->metype, mm->mesub);
    }
  } else {
    if (self->check_crc)
      printf("DF %d with good CRC received "
             "(decoding still not implemented).\n",
        mm->msgtype);
  }
}

int main(int argc, char **argv) {
  adsb_mode_s_t state;
  uint16_t *mag;

  if (argc != 2) {
    fprintf(stderr, "Provide data filename as first argument");
    exit(1);
  }

  char *filename = strdup(argv[1]);
  if ((fd = open(filename, O_RDONLY)) == -1) {
    perror("Opening data file");
    exit(1);
  }

  data_len = MODE_S_DATA_LEN + (MODE_S_FULL_LEN-1)*4;
  if ((data = malloc(data_len)) == NULL ||
    (mag = malloc(data_len*2)) == NULL) {
    fprintf(stderr, "Out of memory allocating data buffer.\n");
    exit(1);
  }

  adsb_init(&state);

  pthread_create(&reader_thread, NULL, reader_thread_entry_point, NULL);

  pthread_mutex_lock(&data_mutex);
  while(1) {
    if (!data_ready) {
      pthread_cond_wait(&data_cond, &data_mutex);
      continue;
    }
    adsb_compute_magnitude_vector(data, mag, data_len);

		// Signal to the other thread that we processed the available data and we
		// want more.
		data_ready = 0;
    pthread_cond_signal(&data_cond);

		// Process data after releasing the lock, so that the capturing thread can
		// read data while we perform computationally expensive stuff * at the same
		// time. (This should only be useful with very slow processors).
		pthread_mutex_unlock(&data_mutex);
    adsb_detect_mode_s(&state, mag, data_len/2, display_mode_s_msg);
    pthread_mutex_lock(&data_mutex);
    if (should_exit) break;
  }
}
