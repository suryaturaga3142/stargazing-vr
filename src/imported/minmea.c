/*
 * Copyright © 2014-2017 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See the COPYING file for more details.
 */

#if defined(__GNUC__) && __GNUC__ >= 4
#   define _XOPEN_SOURCE 600
#   define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#include "minmea.h"

#define boolstr(s) ((s) ? "true" : "false")

static int hex2int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

uint8_t minmea_checksum(const char *sentence)
{
    // Support sentences with or without the starting dollar sign.
    if (*sentence == '$')
        sentence++;

    uint8_t checksum = 0x00;

    // The optional checksum is an XOR of all characters between "$" and "*".
    while (*sentence && *sentence != '*')
        checksum ^= *sentence++;

    return checksum;
}

bool minmea_check(const char *sentence, bool strict)
{
    uint8_t checksum = 0x00;

    // Sequence length constraints.
    if (strlen(sentence) > MINMEA_MAX_LENGTH + 3)
        return false;

    // A valid sentence starts with "$".
    if (*sentence++ != '$')
        return false;

    // The optional checksum is an XOR of all characters between "$" and "*".
    while (*sentence && *sentence != '*')
        checksum ^= *sentence++;

    // If checksum is present...
    if (*sentence == '*') {
        // Checksum is a two-character hexadecimal value.
        sentence++;
        int high = hex2int(*sentence++);
        if (high == -1)
            return false;
        int low = hex2int(*sentence++);
        if (low == -1)
            return false;
        int expected = (high << 4) | low;

        // Check for checksum mismatch.
        if (checksum != expected)
            return false;
    } else if (strict) {
        // Discard sentence if strict verification is enabled and checksum is
        // missing.
        return false;
    }

    // The sentence must end at the checksum calculation result.
    return (*sentence == '\0' || *sentence == '\r' || *sentence == '\n');
}

static inline bool minmea_isfield(char c) {
    return isprint((unsigned char) c) && c != ',' && c != '*';
}

bool minmea_scan(const char *sentence, const char *format, ...)
{
    bool result = false;
    bool optional = false;
    va_list ap;
    va_start(ap, format);

    const char *field = sentence;
    if (*field == '$')
        field++;

    while (*format) {
        char type = *format++;

        if (type == ';') {
            // All further fields are optional.
            optional = true;
            continue;
        }

        if (!*field) {
            if (optional) {
                result = true;
                break;
            } else {
                goto parse_error;
            }
        }

        if (type == 'c') { // Single character field (char).
            char value = *field;

            if (value == ',' || value == '*') {
                if (optional) {
                    field++;
                    continue;
                } else {
                    goto parse_error;
                }
            }

            if (!minmea_isfield(value))
                goto parse_error;

            char *p = va_arg(ap, char *);
            *p = value;
            field++;
        } else if (type == 'd') { // Integer field (int).
            int value = 0;

            if (*field == ',' || *field == '*') {
                if (optional) {
                    field++;
                    continue;
                } else {
                    goto parse_error;
                }
            }

            char *endptr;
            value = strtol(field, &endptr, 10);
            if (field == endptr)
                goto parse_error;
            if (*endptr != ',' && *endptr != '*') {
                if (!optional)
                    goto parse_error;
            }

            int *p = va_arg(ap, int *);
            *p = value;
            field = endptr;
        } else if (type == 'f') { // Fractional field (struct minmea_float).
            if (*field == ',' || *field == '*') {
                if (optional) {
                    field++;
                    continue;
                } else {
                    goto parse_error;
                }
            }

            char *endptr;
            int_least32_t value = strtol(field, &endptr, 10);
            int_least32_t scale = 1;

            if (*endptr == '.') {
                endptr++;
                char *fract = endptr;
                while (isdigit((unsigned char) *endptr))
                    endptr++;
                for (int i = 0; i < endptr - fract; i++)
                    scale *= 10;
                int_least32_t fract_val = strtol(fract, &endptr, 10);
                value = value * scale + (value >= 0 ? fract_val : -fract_val);
            }

            if (field == endptr)
                goto parse_error;
            if (*endptr != ',' && *endptr != '*') {
                if (!optional)
                    goto parse_error;
            }

            struct minmea_float *p = va_arg(ap, struct minmea_float *);
            p->value = value;
            p->scale = scale;
            field = endptr;
        } else if (type == 'i') { // Integer field, given as hex (int).
            int value = 0;

            if (*field == ',' || *field == '*') {
                if (optional) {
                    field++;
                    continue;
                } else {
                    goto parse_error;
                }
            }

            char *endptr;
            value = strtol(field, &endptr, 16);
            if (field == endptr)
                goto parse_error;
            if (*endptr != ',' && *endptr != '*') {
                if (!optional)
                    goto parse_error;
            }

            int *p = va_arg(ap, int *);
            *p = value;
            field = endptr;
        } else if (type == 's') { // String field (char *).
            if (*field == ',' || *field == '*') {
                if (optional) {
                    field++;
                    continue;
                } else {
                    goto parse_error;
                }
            }

            char *start = (char *) field;
            while (minmea_isfield(*field))
                field++;
            if (*field != ',' && *field != '*') {
                if (!optional)
                    goto parse_error;
            }

            char *p = va_arg(ap, char *);
            strncpy(p, start, field - start);
            p[field - start] = '\0';
            field++;
        } else if (type == 't') { // Time field (struct minmea_time).
            struct minmea_time *time_ = va_arg(ap, struct minmea_time *);
            time_->hours = -1;
            time_->minutes = -1;
            time_->seconds = -1;
            time_->microseconds = -1;

            if (*field == ',' || *field == '*') {
                if (optional) {
                    field++;
                    continue;
                } else {
                    goto parse_error;
                }
            }

            // Parse hours.
            char *endptr;
            time_->hours = strtol(field, &endptr, 10);
            if (endptr != field + 2)
                goto parse_error;
            field += 2;

            // Parse minutes.
            time_->minutes = strtol(field, &endptr, 10);
            if (endptr != field + 2)
                goto parse_error;
            field += 2;

            // Parse seconds.
            time_->seconds = strtol(field, &endptr, 10);
            if (endptr != field + 2)
                goto parse_error;
            field += 2;

            // Parse microseconds.
            if (*field == '.') {
                field++;
                int scale = 100000;
                time_->microseconds = 0;
                while (isdigit((unsigned char) *field)) {
                    time_->microseconds += (*field++ - '0') * scale;
                    scale /= 10;
                }
            } else {
                time_->microseconds = 0;
            }

            if (*field != ',' && *field != '*') {
                if (!optional)
                    goto parse_error;
            }
            field++;
        } else if (type == 'D') { // Date field (struct minmea_date).
            struct minmea_date *date_ = va_arg(ap, struct minmea_date *);
            date_->day = -1;
            date_->month = -1;
            date_->year = -1;

            if (*field == ',' || *field == '*') {
                if (optional) {
                    field++;
                    continue;
                } else {
                    goto parse_error;
                }
            }

            // Parse day.
            char *endptr;
            date_->day = strtol(field, &endptr, 10);
            if (endptr != field + 2)
                goto parse_error;
            field += 2;

            // Parse month.
            date_->month = strtol(field, &endptr, 10);
            if (endptr != field + 2)
                goto parse_error;
            field += 2;

            // Parse year.
            date_->year = strtol(field, &endptr, 10);
            if (endptr != field + 2)
                goto parse_error;
            date_->year += 2000;
            field += 2;

            if (*field != ',' && *field != '*') {
                if (!optional)
                    goto parse_error;
            }
            field++;
        } else if (type == '_') { // Ignore field.
            if (*field == ',' || *field == '*') {
                if (optional) {
                    field++;
                    continue;
                }
            }

            while (minmea_isfield(*field))
                field++;
            if (*field != ',' && *field != '*') {
                if (!optional)
                    goto parse_error;
            }
            field++;
        } else { // Unknown type.
            goto parse_error;
        }
    }

    result = true;

parse_error:
    va_end(ap);
    return result;
}

enum minmea_sentence_id minmea_sentence_id(const char *sentence, bool strict)
{
    if (!minmea_check(sentence, strict))
        return MINMEA_INVALID;

    char type[6];
    if (!minmea_scan(sentence, "s", type))
        return MINMEA_INVALID;

    if (!strcmp(type+2, "RMC"))
        return MINMEA_SENTENCE_RMC;
    if (!strcmp(type+2, "GGA"))
        return MINMEA_SENTENCE_GGA;
    if (!strcmp(type+2, "GSA"))
        return MINMEA_SENTENCE_GSA;
    if (!strcmp(type+2, "GLL"))
        return MINMEA_SENTENCE_GLL;
    if (!strcmp(type+2, "GST"))
        return MINMEA_SENTENCE_GST;
    if (!strcmp(type+2, "GSV"))
        return MINMEA_SENTENCE_GSV;
    if (!strcmp(type+2, "VTG"))
        return MINMEA_SENTENCE_VTG;
    if (!strcmp(type+2, "ZDA"))
        return MINMEA_SENTENCE_ZDA;

    return MINMEA_UNKNOWN;
}

bool minmea_parse_rmc(struct minmea_sentence_rmc *frame, const char *sentence)
{
    // $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
    char type[6];
    char validity;
    char latitude_dir;
    char longitude_dir;
    char variation_dir;
    if (!minmea_scan(sentence, "stcfcfcfcDfc;c",
            type,
            &frame->time,
            &validity,
            &frame->latitude, &latitude_dir,
            &frame->longitude, &longitude_dir,
            &frame->speed,
            &frame->course,
            &frame->date,
            &frame->variation, &variation_dir,
            &frame->mode_indicator
            ))
        return false;
    if (strcmp(type+2, "RMC"))
        return false;

    frame->valid = (validity == 'A');
    if (frame->latitude.value != 0 || frame->latitude.scale != 0)
      frame->latitude.value *= (latitude_dir == 'N' ? 1 : -1);
    if (frame->longitude.value != 0 || frame->longitude.scale != 0)
      frame->longitude.value *= (longitude_dir == 'E' ? 1 : -1);
    if (frame->variation.value != 0 || frame->variation.scale != 0)
      frame->variation.value *= (variation_dir == 'W' ? -1 : 1);

    return true;
}

bool minmea_parse_gga(struct minmea_sentence_gga *frame, const char *sentence)
{
    // $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
    char type[6];
    char latitude_dir;
    char longitude_dir;
    if (!minmea_scan(sentence, "stcfcifcfcfc;i",
            type,
            &frame->time,
            &frame->latitude, &latitude_dir,
            &frame->longitude, &longitude_dir,
            &frame->fix_quality,
            &frame->satellites_tracked,
            &frame->hdop,
            &frame->altitude, &frame->altitude_units,
            &frame->height, &frame->height_units,
            &frame->dgps_age
            ))
        return false;
    if (strcmp(type+2, "GGA"))
        return false;

    if (frame->latitude.value != 0 || frame->latitude.scale != 0)
      frame->latitude.value *= (latitude_dir == 'N' ? 1 : -1);
    if (frame->longitude.value != 0 || frame->longitude.scale != 0)
      frame->longitude.value *= (longitude_dir == 'E' ? 1 : -1);

    return true;
}

bool minmea_parse_gsa(struct minmea_sentence_gsa *frame, const char *sentence)
{
    // $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
    char type[6];
    if (!minmea_scan(sentence, "sci;iiiiiiiiiiiifff",
            type,
            &frame->mode,
            &frame->fix_type,
            &frame->sats[0],
            &frame->sats[1],
            &frame->sats[2],
            &frame->sats[3],
            &frame->sats[4],
            &frame->sats[5],
            &frame->sats[6],
            &frame->sats[7],
            &frame->sats[8],
            &frame->sats[9],
            &frame->sats[10],
            &frame->sats[11],
            &frame->pdop,
            &frame->hdop,
            &frame->vdop
            )) {
        // Assume default values for optional fields.
        frame->pdop.value = 0; frame->pdop.scale = 0;
        frame->hdop.value = 0; frame->hdop.scale = 0;
        frame->vdop.value = 0; frame->vdop.scale = 0;
    }
    if (strcmp(type+2, "GSA"))
        return false;

    return true;
}

bool minmea_parse_gll(struct minmea_sentence_gll *frame, const char *sentence)
{
    // $GPGLL,4916.45,N,12311.12,W,225444,A,*1D
    char type[6];
    char latitude_dir;
    char longitude_dir;
    if (!minmea_scan(sentence, "scfcfctc;c",
            type,
            &frame->latitude, &latitude_dir,
            &frame->longitude, &longitude_dir,
            &frame->time,
            &frame->status,
            &frame->mode_indicator
            ))
        return false;
    if (strcmp(type+2, "GLL"))
        return false;

    if (frame->latitude.value != 0 || frame->latitude.scale != 0)
      frame->latitude.value *= (latitude_dir == 'N' ? 1 : -1);
    if (frame->longitude.value != 0 || frame->longitude.scale != 0)
      frame->longitude.value *= (longitude_dir == 'E' ? 1 : -1);

    return true;
}

bool minmea_parse_gst(struct minmea_sentence_gst *frame, const char *sentence)
{
    // $GPGST,172814.0,0.006,0.023,0.020,273.6,0.023,0.020,0.031*6A
    char type[6];
    if (!minmea_scan(sentence, "stfffffff",
            type,
            &frame->time,
            &frame->rms_deviation,
            &frame->semi_major_deviation,
            &frame->semi_minor_deviation,
            &frame->semi_major_orientation,
            &frame->latitude_error_deviation,
            &frame->longitude_error_deviation,
            &frame->altitude_error_deviation
            ))
        return false;
    if (strcmp(type+2, "GST"))
        return false;

    return true;
}

bool minmea_parse_gsv(struct minmea_sentence_gsv *frame, const char *sentence)
{
    // $GPGSV,3,1,11,03,03,111,00,04,15,270,00,05,01,220,00,06,01,000,00*79
    char type[6];
    if (!minmea_scan(sentence, "siii;iiii;iiii;iiii;iiii",
            type,
            &frame->total_msgs,
            &frame->msg_nr,
            &frame->total_sats,
            &frame->sats[0].nr,
            &frame->sats[0].elevation,
            &frame->sats[0].azimuth,
            &frame->sats[0].snr,
            &frame->sats[1].nr,
            &frame->sats[1].elevation,
            &frame->sats[1].azimuth,
            &frame->sats[1].snr,
            &frame->sats[2].nr,
            &frame->sats[2].elevation,
            &frame->sats[2].azimuth,
            &frame->sats[2].snr,
            &frame->sats[3].nr,
            &frame->sats[3].elevation,
            &frame->sats[3].azimuth,
            &frame->sats[3].snr
            )) {
        // Assume default values for optional fields.
        frame->sats[0].nr = -1;
        frame->sats[1].nr = -1;
        frame->sats[2].nr = -1;
        frame->sats[3].nr = -1;
    }
    if (strcmp(type+2, "GSV"))
        return false;

    return true;
}

bool minmea_parse_vtg(struct minmea_sentence_vtg *frame, const char *sentence)
{
    // $GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
    char type[6];
    char true_track_units;
    char magnetic_track_units;
    char speed_knots_units;
    char speed_kph_units;
    if (!minmea_scan(sentence, "sfcfcfcfc;c",
            type,
            &frame->true_track_degrees, &true_track_units,
            &frame->magnetic_track_degrees, &magnetic_track_units,
            &frame->speed_knots, &speed_knots_units,
            &frame->speed_kph, &speed_kph_units,
            &frame->mode_indicator
            )) {
        // Assume default values for optional fields.
        frame->mode_indicator = 0;
    }
    if (strcmp(type+2, "VTG"))
        return false;
    if (true_track_units != 'T')
        return false;
    if (magnetic_track_units != 'M')
        return false;
    if (speed_knots_units != 'N')
        return false;
    if (speed_kph_units != 'K')
        return false;

    return true;
}

bool minmea_parse_zda(struct minmea_sentence_zda *frame, const char *sentence)
{
    // $GPZDA,201530.00,04,07,2002,00,00*60
    char type[6];
    if (!minmea_scan(sentence, "stDii",
            type,
            &frame->time,
            &frame->date,
            &frame->hour_offset,
            &frame->minute_offset
            ))
        return false;
    if (strcmp(type+2, "ZDA"))
        return false;

    // Support ZDA sentences without local time zone.
    if (frame->hour_offset == 0 && frame->minute_offset == 0) {
        char *field = (char *) sentence;
        for (int i = 0; i < 5; i++) {
            while (minmea_isfield(*field))
                field++;
            field++;
        }
        if (*field == ',' || *field == '*') {
            frame->hour_offset = 0;
            frame->minute_offset = 0;
        }
    }

    return true;
}

int_least32_t minmea_rescale(struct minmea_float *f, int_least32_t new_scale)
{
    if (f->scale == 0)
        return 0;
    if (f->scale == new_scale)
        return f->value;
    if (f->scale > new_scale)
        return (f->value + ((f->value > 0 ? 1 : -1) * f->scale / new_scale / 2)) / (f->scale / new_scale);
    else
        return f->value * (new_scale / f->scale);
}

#if !defined(MINMEA_NO_TIME)
#if defined(MINMEA_WIN32)
#include <windows.h>
#ifndef LEAP_SECONDS_SINCE_1980
/* Windows-specific leap second count. */
#define LEAP_SECONDS_SINCE_1980 18
#endif
int minmea_gettime(struct tm *tm, const struct minmea_date *date, const struct minmea_time *time_)
{
    SYSTEMTIME st = {
        .wYear = date->year,
        .wMonth = date->month,
        .wDay = date->day,
        .wHour = time_->hours,
        .wMinute = time_->minutes,
        .wSecond = time_->seconds,
    };
    FILETIME ft;
    if (!SystemTimeToFileTime(&st, &ft))
        return -1;
    LARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    /* Convert to seconds since 1970. Based on:
     * https://support.microsoft.com/en-us/help/167296/how-to-convert-a-unix-time-t-to-a-win32-filetime-or-systemtime
     */
    time_t timestamp = (li.QuadPart - 116444736000000000LL) / 10000000LL - LEAP_SECONDS_SINCE_1980;
    *tm = *gmtime(&timestamp);
    return 0;
}
#else
int minmea_gettime(struct tm *tm, const struct minmea_date *date, const struct minmea_time *time_)
{
    memset(tm, 0, sizeof(*tm));
    tm->tm_year = date->year - 1900;
    tm->tm_mon = date->month - 1;
    tm->tm_mday = date->day;
    tm->tm_hour = time_->hours;
    tm->tm_min = time_->minutes;
    tm->tm_sec = time_->seconds;
    time_t timestamp = timegm(tm); /* See README.md if your system lacks timegm(). */
    if (timestamp == (time_t)-1)
        return -1;
    *tm = *gmtime(&timestamp);
    return 0;
}
#endif
#endif /* !MINMEA_NO_TIME */

/* vim: set ts=4 sw=4 et: */
