#ifndef CONFIG_H
#define CONFIG_H

/* ===================== PIN DEFINITIONS (STM32F401) ===================== */
#define PPM_PIN           PA0     // Receiver connection PPM
#define ESC_PORT_PIN      PB6     // ESC port (babord)
#define ESC_STARBOARD_PIN PB7     // ESC starboard (tribord)
#define LED_GPS           PC13    // GPS status LED

/* ===================== SERIAL ===================== */
#define GPS_BAUD          115200  // GPS serial baud rate
#define TELEMETRY_BAUD    57600   // Telemetry serial baud rate
#define DEBUG_BAUD        115200  // USB debug serial

/* ===================== CONSTANTS ===================== */
#define PPM_CHANNELS      8       // Total PPM channels available
#define RC_MIN            1000    // RC min pulse width
#define RC_MAX            2000    // RC max pulse width
#define RC_NEUTRAL        1500    // RC neutral pulse width

#define SYNC_THRESHOLD    3000    // PPM sync threshold (microseconds)
#define MIN_VALID_PULSE   600     // Min valid PPM pulse
#define MAX_VALID_PULSE   2500    // Max valid PPM pulse

#define GPS_HDOP_MAX      1.0     // Max HDOP for good fix

/* ===================== CONTROL PARAMETERS ===================== */
#define HOLD_RADIUS_CALM           2.0   // Calm waters holding radius (meters)
#define HOLD_RADIUS_LOW_CURRENT    3.0   // Low current holding radius
#define HOLD_RADIUS_STRONG_CURRENT 3.5   // Strong current holding radius

#define DIST_GAIN_CALM           1.5
#define DIST_GAIN_LOW_CURRENT    1.8
#define DIST_GAIN_STRONG_CURRENT 2.0

#define YAW_GAIN_CALM           1.4
#define YAW_GAIN_LOW_CURRENT    1.2
#define YAW_GAIN_STRONG_CURRENT 1.0

#define MAX_THRUST_CALM           300
#define MAX_THRUST_LOW_CURRENT    360
#define MAX_THRUST_STRONG_CURRENT 420

#define MIN_THRUST           120
#define MAX_YAW              180

/* ===================== CURRENT ESTIMATION ===================== */
#define CURRENT_SAMPLE_TIME_MS    5000   // Sample time for current estimation
#define CURRENT_LOCK_THRESHOLD    3.0    // Meters/sec threshold for current lock

/* ===================== MAG CALIBRATION ===================== */
#define MAG_OFFSET_X      -12.4   // Hard-iron offset X
#define MAG_OFFSET_Y      8.9     // Hard-iron offset Y
#define MAG_SCALE_X       1.08    // Soft-iron scale X
#define MAG_SCALE_Y       0.94    // Soft-iron scale Y
#define MAG_DECLINATION   2.5     // Declination for Paris area (degrees)

/* ===================== TELEMETRY ===================== */
#define TELEMETRY_INTERVAL_MS    1000   // Send telemetry every 1 second

/* ===================== RC CHANNEL MAPPING ===================== */
#define CH_STEER          0  // Channel 1: Steering (port/starboard)
#define CH_AUX1           1  // Channel 2: Auxiliary
#define CH_THROTTLE       2  // Channel 3: Throttle
#define CH_AUX2           3  // Channel 4: Auxiliary
#define CH_MODE           4  // Channel 5: Autonomous/Manual mode
#define CH_CURRENT_TYPE   5  // Channel 6: Current type selector

/* ===================== RC MODE THRESHOLDS ===================== */
#define RC_CALM_THRESHOLD_LOW    1300
#define RC_CALM_THRESHOLD_HIGH   1700
#define RC_MODE_THRESHOLD        1500

#endif // CONFIG_H
