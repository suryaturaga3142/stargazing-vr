/*
 * Copyright © 2014-2017 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See the COPYING file for more details.
 */

#ifndef MINMEA_H
#define MINMEA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#if defined(__GNUC__) && __GNUC__ >= 4
#   define MINMEA_API __attribute__((visibility("default")))
#else
#   define MINMEA_API
#endif

#define MINMEA_MAX_LENGTH 82

enum minmea_sentence_id {
    MINMEA_INVALID = -1,
    MINMEA_UNKNOWN = 0,
    MINMEA_SENTENCE_RMC,
    MINMEA_SENTENCE_GGA,
    MINMEA_SENTENCE_GSA,
    MINMEA_SENTENCE_GLL,
    MINMEA_SENTENCE_GST,
    MINMEA_SENTENCE_GSV,
    MINMEA_SENTENCE_VTG,
    MINMEA_SENTENCE_ZDA,
};

struct minmea_float {
    int_least32_t value;
    int_least32_t scale;
};

struct minmea_date {
    int day;
    int month;
    int year;
};

struct minmea_time {
    int hours;
    int minutes;
    int seconds;
    int microseconds;
};

struct minmea_sentence_rmc {
    struct minmea_time time;
    bool valid;
    struct minmea_float latitude;
    struct minmea_float longitude;
    struct minmea_float speed;
    struct minmea_float course;
    struct minmea_date date;
    struct minmea_float variation;
    char mode_indicator;
};

struct minmea_sentence_gga {
    struct minmea_time time;
    struct minmea_float latitude;
    struct minmea_float longitude;
    int fix_quality;
    int satellites_tracked;
    struct minmea_float hdop;
    struct minmea_float altitude; char altitude_units;
    struct minmea_float height; char height_units;
    int dgps_age;
};

enum minmea_gsa_mode {
    MINMEA_GPGSA_MODE_AUTO = 'A',
    MINMEA_GPGSA_MODE_MANUAL = 'M',
};

enum minmea_gsa_fix_type {
    MINMEA_GPGSA_FIX_NONE = 1,
    MINMEA_GPGSA_FIX_2D = 2,
    MINMEA_GPGSA_FIX_3D = 3,
};

struct minmea_sentence_gsa {
    char mode;
    int fix_type;
    int sats[12];
    struct minmea_float pdop;
    struct minmea_float hdop;
    struct minmea_float vdop;
};

struct minmea_sentence_gll {
    struct minmea_float latitude;
    struct minmea_float longitude;
    struct minmea_time time;
    char status;
    char mode_indicator;
};

struct minmea_sentence_gst {
    struct minmea_time time;
    struct minmea_float rms_deviation;
    struct minmea_float semi_major_deviation;
    struct minmea_float semi_minor_deviation;
    struct minmea_float semi_major_orientation;
    struct minmea_float latitude_error_deviation;
    struct minmea_float longitude_error_deviation;
    struct minmea_float altitude_error_deviation;
};

struct minmea_sat_info {
    int nr;
    int elevation;
    int azimuth;
    int snr;
};

struct minmea_sentence_gsv {
    int total_msgs;
    int msg_nr;
    int total_sats;
    struct minmea_sat_info sats[4];
};

struct minmea_sentence_vtg {
    struct minmea_float true_track_degrees;
    struct minmea_float magnetic_track_degrees;
    struct minmea_float speed_knots;
    struct minmea_float speed_kph;
    char mode_indicator;
};

struct minmea_sentence_zda {
    struct minmea_time time;
    struct minmea_date date;
    int hour_offset;
    int minute_offset;
};

/**
 * Calculate sentence checksum.
 */
MINMEA_API uint8_t minmea_checksum(const char *sentence);

/**
 * Check sentence validity and checksum.
 */
MINMEA_API bool minmea_check(const char *sentence, bool strict);

/**
 * Determine sentence identifier.
 */
MINMEA_API enum minmea_sentence_id minmea_sentence_id(const char *sentence, bool strict);

/**
 * Scanf-like processor for NMEA sentences.
 */
MINMEA_API bool minmea_scan(const char *sentence, const char *format, ...);

/**
 * Parse a specific sentence type.
 */
MINMEA_API bool minmea_parse_rmc(struct minmea_sentence_rmc *frame, const char *sentence);
MINMEA_API bool minmea_parse_gga(struct minmea_sentence_gga *frame, const char *sentence);
MINMEA_API bool minmea_parse_gsa(struct minmea_sentence_gsa *frame, const char *sentence);
MINMEA_API bool minmea_parse_gll(struct minmea_sentence_gll *frame, const char *sentence);
MINMEA_API bool minmea_parse_gst(struct minmea_sentence_gst *frame, const char *sentence);
MINMEA_API bool minmea_parse_gsv(struct minmea_sentence_gsv *frame, const char *sentence);
MINMEA_API bool minmea_parse_vtg(struct minmea_sentence_vtg *frame, const char *sentence);
MINMEA_API bool minmea_parse_zda(struct minmea_sentence_zda *frame, const char *sentence);

/**
 * Convert GPS UTC time to a standard time_t value.
 * This is not available on all platforms, see MINMEA_NO_TIME.
 */
MINMEA_API int minmea_gettime(struct tm *tm, const struct minmea_date *date, const struct minmea_time *time_);

/**
 * Rescale a fixed-point value to a different scale.
 */
MINMEA_API int_least32_t minmea_rescale(struct minmea_float *f, int_least32_t new_scale);

/**
 * Convert a fixed-point value to floating point.
 */
static inline float minmea_tofloat(struct minmea_float *f)
{
    if (f->scale == 0)
        return NAN;
    return (float) f->value / (float) f->scale;
}

/**
 * Convert a fixed-point value to coordinate.
 */
static inline float minmea_tocoord(struct minmea_float *f)
{
    if (f->scale == 0)
        return NAN;
    int_least32_t degrees = f->value / (f->scale * 100);
    int_least32_t minutes = f->value % (f->scale * 100);
    return (float) degrees + (float) minutes / (60 * (float) f->scale);
}

#ifdef __cplusplus
}
#endif

#endif /* MINMEA_H */

/* vim: set ts=4 sw=4 et: */
