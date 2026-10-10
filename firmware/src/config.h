/* Build-time settings. */
#ifndef CONFIG_H
#define CONFIG_H

/* The tag's status link; the page offers a one-tap time sync on Android (Web NFC). */
#define STATUS_URL "captainchapster.github.io/Eigentime32/sync/"

#define DISPLAY_TIMEOUT_S 60   /* display off after this long without a button press */
#define LONG_PRESS_MS 1000
#define REPEAT_MS 180
#define MESSAGE_S 4            /* how long a status message stays up */
#define MIN_DRIFT_SPAN_S (7L * 86400) /* don't trust a rate measured over less than a week */
#define STATUS_REFRESH_S (6L * 3600)  /* rewrite the tag's status link at most this often */

#endif
