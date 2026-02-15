#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

/* ===================== DEBUG OPTIONS ===================== */
// Uncomment to skip compass initialization if it's causing issues
// #define SKIP_COMPASS_INIT

// Uncomment to skip GPS wait for lab testing without GPS signal
#define SKIP_GPS_WAIT

// Debug output destination: comment out to use USB Serial, uncomment to use HC12
#define USE_HC12_DEBUG
#ifdef USE_HC12_DEBUG
  extern HardwareSerial SerialTelemetry;  // Declare external serial for HC12
  #define DEBUG_SERIAL SerialTelemetry
  #define DEBUG_BAUD   19200  // HC12 serial baud rate
#else
  #define DEBUG_SERIAL Serial
  #define DEBUG_BAUD   19200 // USB debug serial
#endif

/* ===================== PIN DEFINITIONS (STM32F401) ===================== */
#define PPM_PIN           PB0     // Receiver connection PPM (on GPIOB to avoid PA interference)
#define ESC_PORT_PIN      PB6     // ESC port (babord)
#define ESC_STARBOARD_PIN PB7     // ESC starboard (tribord)
#define LED_GPS           PC13    // GPS status LED
#define GPS_TX            PA2     // GPS module TX → STM32 RX (USART2)
#define GPS_RX            PA3     // GPS module RX → STM32 TX (USART2)
#define TELEM_TX          PA9     // Telemetry TX (USART1) - Also HC12 when USE_HC12_DEBUG - HC12 RX pin
#define TELEM_RX          PA10    // Telemetry RX (USART1) - Also HC12 when USE_HC12_DEBUG - HC12 TX pin
#define MAG_SCL           PB8     // Magnetometer I2C SCL
#define MAG_SDA           PB9     // Magnetometer I2C SDA

// Gyro SPI pins (ICM-42688-P)
#define GYRO_CS           PA4     // Gyro SPI Chip Select
// SPI1: SCK=PA5, MISO=PA6, MOSI=PA7 (STM32 hardware SPI1)

/* ===================== SERIAL ===================== */
#define GPS_BAUD          9600    // GPS serial baud rate
#define TELEMETRY_BAUD    19200   // Telemetry serial baud rate (shared with HC12 debug)

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

/* ===================== GYRO YAW RATE DAMPING ===================== */
// Yaw rate damping gains (prevents spinning)
#define YAW_RATE_GAIN_CALM           0.8   // Lower gain for calm waters
#define YAW_RATE_GAIN_LOW_CURRENT    1.2   // Medium gain for low current
#define YAW_RATE_GAIN_STRONG_CURRENT 1.5   // Higher gain for strong current

// Maximum allowed yaw rate before damping kicks in (deg/s)
#define YAW_RATE_DEADBAND            5.0   // Deadband to avoid fighting small oscillations

// Gyro update rate
#define GYRO_UPDATE_INTERVAL_MS      20    // 50 Hz update rate

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
#define CH_CALIBRATION    6  // Channel 7: Calibration mode (compass/off/gyro)

/* ===================== RC MODE THRESHOLDS ===================== */
#define RC_CALM_THRESHOLD_LOW    1300
#define RC_CALM_THRESHOLD_HIGH   1700
#define RC_MODE_THRESHOLD        1500

// Calibration switch thresholds (3-position)
#define RC_CAL_COMPASS_MAX       1400   // < 1400 = Compass calibration
#define RC_CAL_GYRO_MIN          1600   // > 1600 = Gyro calibration
// Between 1400-1600 = No calibration

#endif // CONFIG_H