// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Shannon Smith
/* Exact provider-owned lunar years; ICU supplies localized text and the
 * explicitly retained projection outside the maintained year table. */
#include <infiltratr/temporal_calendar.h>
#include <glib.h>
#include <stdint.h>
#include <string.h>
#include "calendar-types.h"
#include "calendar-engine-types.h"
#include "icu-calendar.h"
#include "catalogue.h"
#include "julian-day.h"
#include "lunar-data.h"

typedef struct LunarMonth {
    int64_t start, year;
    int month, length;
    gboolean leap;
} LunarMonth;

static int64_t floor_divide(int64_t value, int64_t divisor)
{
    return value / divisor - (value % divisor < 0 ? 1 : 0);
}

static int64_t july_first(int64_t year)
{
    /* All ICU-supported civil years fit this bound; reject before products. */
    if (year < -300000 || year > 300000) return G_MAXINT64;
    const int64_t previous = year - 1;
    const gboolean leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    return INT64_C(1721426) + previous * 365 + floor_divide(previous, 4) -
        floor_divide(previous, 100) + floor_divide(previous, 400) + 181 + (leap ? 1 : 0);
}

static int64_t related_year(const CalendarCalendarFields *f)
{
    return ((int64_t)f->auxiliary - 1) * 60 + f->year - 1 - 2636;
}

static gboolean month_at(int64_t index, LunarMonth *out)
{
    if (out == NULL || index < 0 || index >= LUNAR_MONTH_COUNT) return FALSE;
    size_t low = 0, high = G_N_ELEMENTS(lunar_years);
    while (low + 1 < high) {
        const size_t mid = low + (high - low) / 2;
        if (lunar_prefix[mid] <= index) low = mid; else high = mid;
    }
    const LunarYear *year = &lunar_years[low];
    const unsigned ordinal = (unsigned)(index - lunar_prefix[low]);
    int64_t start = year->start;
    for (unsigned i = 0; i < ordinal; ++i) start += 29 + ((year->lengths >> i) & 1U);
    const unsigned number = ordinal + 1U;
    *out = (LunarMonth){ start, LUNAR_FIRST_YEAR + (int64_t)low,
        (int)(number - (year->leap != 0U && number >= year->leap ? 1U : 0U)),
        29 + (int)((year->lengths >> ordinal) & 1U),
        year->leap != 0U && number == year->leap };
    return TRUE;
}

static gboolean month_key(int64_t year, int month, gboolean leap,
                          int64_t *index, LunarMonth *out)
{
    if (year < LUNAR_FIRST_YEAR || year > LUNAR_LAST_YEAR) return FALSE;
    const size_t y = (size_t)(year - LUNAR_FIRST_YEAR);
    for (int64_t i = lunar_prefix[y]; i < lunar_prefix[y + 1U]; ++i) {
        LunarMonth candidate;
        if (!month_at(i, &candidate)) return FALSE;
        if (candidate.month == month && candidate.leap == leap) {
            if (index != NULL) *index = i;
            if (out != NULL) *out = candidate;
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean date_at(int64_t jdn, CalendarCalendarFields *out,
                         int64_t *index)
{
    if (jdn < lunar_years[0].start || jdn >= LUNAR_END_JDN) return FALSE;
    int64_t low = 0, high = LUNAR_MONTH_COUNT;
    while (low + 1 < high) {
        const int64_t mid = low + (high - low) / 2;
        LunarMonth candidate;
        if (!month_at(mid, &candidate)) return FALSE;
        if (candidate.start <= jdn) low = mid; else high = mid;
    }
    LunarMonth month;
    if (!month_at(low, &month)) return FALSE;
    if (out != NULL) *out = (CalendarCalendarFields){
        (month.year - 4) % 60 + 1, month.month,
        (int)(jdn - month.start + 1), (int)((month.year + 2636) / 60 + 1), month.leap };
    if (index != NULL) *index = low;
    return TRUE;
}

static gboolean chronology_fields(int64_t jdn, CalendarCalendarFields *out)
{
    return date_at(jdn, out, NULL) ||
        calendar_icu_fields_from_jdn(RESOURCE_KEYWORD, jdn, out);
}

/* Locate a logical month in the legacy projection. Enumeration begins in
 * July of the related Gregorian year, then walks to that lunar new year.
 * Every result is checked before it is used as a chronology anchor. */
static gboolean projected_month(int64_t year, int month, gboolean leap,
                                  LunarMonth *out)
{
    const int64_t probe = july_first(year);
    CalendarCalendarFields f;
    if (!calendar_icu_fields_from_jdn(RESOURCE_KEYWORD, probe, &f) ||
        related_year(&f) != year) return FALSE;
    int64_t start = calendar_icu_month_start(RESOURCE_KEYWORD, probe);
    for (int i = 0; i < 14; ++i) {
        if (!calendar_icu_fields_from_jdn(RESOURCE_KEYWORD, start, &f)) return FALSE;
        if (f.month == 1 && !f.special) break;
        start = calendar_icu_add_months(RESOURCE_KEYWORD, start, -1);
    }
    for (int i = 0; i < 14; ++i) {
        if (!calendar_icu_fields_from_jdn(RESOURCE_KEYWORD, start, &f) ||
            related_year(&f) != year || f.day != 1) return FALSE;
        const int64_t next = calendar_icu_add_months(RESOURCE_KEYWORD, start, 1);
        if (next == G_MAXINT64 || next <= start || next - start < 29 || next - start > 30) return FALSE;
        if (f.month == month && f.special == leap) {
            *out = (LunarMonth){ start, year, month, (int)(next - start), leap };
            return TRUE;
        }
        start = next;
    }
    return FALSE;
}

static gboolean anchors(int64_t *first, int64_t *after)
{
    LunarMonth initial, final, projected;
    if (!month_at(0, &initial) || !month_at(LUNAR_MONTH_COUNT - 1, &final) ||
        !projected_month(initial.year, initial.month, initial.leap, &projected)) return FALSE;
    *first = projected.start;
    if (!projected_month(final.year, final.month, final.leap, &projected)) return FALSE;
    *after = calendar_icu_add_months(RESOURCE_KEYWORD, projected.start, 1);
    return *after != G_MAXINT64;
}

static bool fields(const InfiltratrTemporalCalendarProvider *self,
                         int64_t jdn, InfiltratrTemporalCalendarFields *out)
{
    (void)self;
    CalendarCalendarFields f;
    if (out == NULL || !chronology_fields(jdn, &f)) return FALSE;
    *out = (InfiltratrTemporalCalendarFields){ f.year, f.month, f.day, f.auxiliary, f.special != FALSE };
    return TRUE;
}

static bool period_start(const InfiltratrTemporalCalendarProvider *self,
                               int64_t jdn, int64_t *out)
{
    (void)self;
    CalendarCalendarFields f, check;
    if (out == NULL || !chronology_fields(jdn, &f)) return FALSE;
    const int64_t candidate = jdn - f.day + 1;
    if (!chronology_fields(candidate, &check) || check.day != 1 ||
        check.year != f.year || check.month != f.month ||
        check.special != f.special || check.auxiliary != f.auxiliary) return FALSE;
    *out = candidate;
    return TRUE;
}

static bool periods(const InfiltratrTemporalCalendarProvider *self,
                          int64_t jdn, int32_t amount, int64_t *out)
{
    (void)self;
    CalendarCalendarFields original, check;
    int64_t index;
    if (out == NULL || !chronology_fields(jdn, &original)) return FALSE;
    if (amount == 0) { *out = jdn; return TRUE; }
    if (!date_at(jdn, NULL, NULL)) {
        /* Retain the established projection for operations whose two
         * endpoints are outside the owned table. Do not route a remote
         * proleptic date through an unnecessary modern-month anchor. */
        const int64_t projected = calendar_icu_add_months(RESOURCE_KEYWORD, jdn, amount);
        if (projected != G_MAXINT64 && !date_at(projected, NULL, NULL) &&
            chronology_fields(projected, &check)) {
            *out = projected;
            return TRUE;
        }
    }
    int64_t first = 0, after = 0;
    if (!date_at(jdn, NULL, &index)) {
        if (!anchors(&first, &after)) return FALSE;
        const int64_t source = calendar_icu_month_start(RESOURCE_KEYWORD, jdn);
        int32_t difference;
        if (!calendar_icu_month_difference(RESOURCE_KEYWORD, source,
                jdn < lunar_years[0].start ? first : after, &difference)) return FALSE;
        index = jdn < lunar_years[0].start ? -(int64_t)difference
                                         : LUNAR_MONTH_COUNT - (int64_t)difference;
    }
    const int64_t target = index + amount;
    LunarMonth month;
    int64_t candidate;
    if (month_at(target, &month)) {
        candidate = month.start + MIN(original.day, month.length) - 1;
    } else {
        if (first == 0 && !anchors(&first, &after)) return FALSE;
        const int64_t delta = target < 0 ? target : target - LUNAR_MONTH_COUNT;
        if (delta < G_MININT32 || delta > G_MAXINT32) return FALSE;
        const int64_t start = calendar_icu_add_months(RESOURCE_KEYWORD,
            target < 0 ? first : after, (int32_t)delta);
        const int64_t next = calendar_icu_add_months(RESOURCE_KEYWORD, start, 1);
        if (start == G_MAXINT64 || next == G_MAXINT64 || next <= start || next - start > 30) return FALSE;
        candidate = start + MIN(original.day, (int)(next - start)) - 1;
    }
    if (!chronology_fields(candidate, &check)) return FALSE;
    *out = candidate;
    return TRUE;
}

static bool years(const InfiltratrTemporalCalendarProvider *self,
                        int64_t jdn, int32_t amount, int64_t *out)
{
    (void)self;
    CalendarCalendarFields original, check;
    if (out == NULL || !chronology_fields(jdn, &original)) return FALSE;
    if (amount == 0) { *out = jdn; return TRUE; }
    if (!date_at(jdn, NULL, NULL)) {
        const int64_t projected = calendar_icu_add_years(RESOURCE_KEYWORD, jdn, amount);
        if (projected != G_MAXINT64 && !date_at(projected, NULL, NULL) &&
            chronology_fields(projected, &check)) {
            *out = projected;
            return TRUE;
        }
    }
    int64_t target_year = related_year(&original) + amount;
    int target_month = original.month;
    gboolean target_leap = original.special;
    LunarMonth month;
    gboolean found = month_key(target_year, target_month, target_leap, NULL, &month);
    if (!found && (target_year < LUNAR_FIRST_YEAR || target_year > LUNAR_LAST_YEAR))
        found = projected_month(target_year, target_month, target_leap, &month);
    /* Explicit year-add normalization: an absent leap month maps to the
     * following ordinary month. This keeps the tested native-range policy
     * deterministic instead of inheriting ICU leniency inconsistencies. */
    if (!found && target_leap) {
        target_leap = FALSE;
        if (++target_month > 12) { target_month = 1; ++target_year; }
        found = month_key(target_year, target_month, target_leap, NULL, &month);
        if (!found && (target_year < LUNAR_FIRST_YEAR || target_year > LUNAR_LAST_YEAR))
            found = projected_month(target_year, target_month, target_leap, &month);
    }
    if (!found) return FALSE;
    const int64_t candidate = month.start + MIN(original.day, month.length) - 1;
    if (!chronology_fields(candidate, &check)) return FALSE;
    *out = candidate;
    return TRUE;
}

static gsize samples_ready;
static int64_t month_samples[12][2];
static void prepare_samples(void)
{
    if (g_once_init_enter(&samples_ready)) {
        gboolean needed[12][2] = {{ FALSE }};
        unsigned remaining = 0;
        for (int64_t i = 0; i < LUNAR_MONTH_COUNT; ++i) {
            LunarMonth month;
            if (month_at(i, &month) && !needed[month.month - 1][month.leap ? 1 : 0]) {
                needed[month.month - 1][month.leap ? 1 : 0] = TRUE; ++remaining;
            }
        }
        int64_t start = calendar_icu_month_start(RESOURCE_KEYWORD, lunar_years[0].start);
        for (int i = 0; i < 2600 && remaining != 0U; ++i) {
            CalendarCalendarFields f;
            if (!calendar_icu_fields_from_jdn(RESOURCE_KEYWORD, start, &f) || f.month < 1 || f.month > 12) break;
            const int leap = f.special ? 1 : 0;
            if (needed[f.month - 1][leap] && month_samples[f.month - 1][leap] == 0) {
                month_samples[f.month - 1][leap] = start; --remaining;
            }
            const int64_t next = calendar_icu_add_months(RESOURCE_KEYWORD, start, 1);
            if (next == G_MAXINT64 || next <= start) break;
            start = next;
        }
        g_once_init_leave(&samples_ready, 1);
    }
}

static gchar *owned_text(int64_t jdn, CalendarDatePart part,
                          const CalendarCalendarFields *f)
{
    prepare_samples();
    const int64_t month_sample = month_samples[f->month - 1][f->special ? 1 : 0];
    if (month_sample == 0) return NULL;
    /* Numbers and month/cycle names are localized separately. ICU is not
     * permitted to substitute its approximate date for the owned chronology. */
    int64_t day_sample = INT64_C(2451580); /* 2000-02-05; a 30-day lunar month */
    CalendarCalendarFields probe;
    gboolean day_found = FALSE;
    for (int i = 0; i < 14; ++i) {
        if (day_sample == G_MAXINT64) break;
        if (calendar_icu_fields_from_jdn(RESOURCE_KEYWORD, day_sample + f->day - 1, &probe) && probe.day == f->day) {
            day_found = TRUE; break;
        }
        day_sample = calendar_icu_add_months(RESOURCE_KEYWORD, day_sample, 1);
    }
    if (!day_found) return NULL;
    const int64_t year_sample = july_first(related_year(f));
    gchar *day = calendar_icu_format_pattern(RESOURCE_KEYWORD, day_sample + f->day - 1, "d");
    gchar *month = calendar_icu_format_pattern(RESOURCE_KEYWORD, month_sample,
        part == CALENDAR_DATE_PART_MONTH ? "LLLL" : "MMMM");
    gchar *year = calendar_icu_format_pattern(RESOURCE_KEYWORD, year_sample, "U");
    gchar *weekday = calendar_icu_format_pattern(RESOURCE_KEYWORD, jdn, "EEEE");
    gchar *text = NULL;
    if (day && *day && month && *month && year && *year && weekday && *weekday) {
        switch (part) {
        case CALENDAR_DATE_PART_DAY: text = g_strdup(day); break;
        case CALENDAR_DATE_PART_MONTH: text = g_strdup(month); break;
        case CALENDAR_DATE_PART_YEAR: text = g_strdup(year); break;
        case CALENDAR_DATE_PART_SHORT: text = g_strdup_printf("%s %s %s", day, month, year); break;
        case CALENDAR_DATE_PART_FULL: text = g_strdup_printf("%s, %s %s %s", weekday, day, month, year); break;
        default: break;
        }
    }
    g_free(day); g_free(month); g_free(year); g_free(weekday);
    return text;
}

static bool format(const InfiltratrTemporalCalendarProvider *self,
                         int64_t jdn, InfiltratrTemporalCalendarDatePart part,
                         char *buffer, size_t capacity, size_t *length)
{
    (void)self;
    if (!buffer || capacity == 0U || part < 1 || part > 5) return FALSE;
    CalendarCalendarFields f;
    if (!chronology_fields(jdn, &f)) return FALSE;
    gchar *text = date_at(jdn, NULL, NULL)
        ? owned_text(jdn, (CalendarDatePart)part, &f)
        : calendar_icu_format(RESOURCE_MODE, RESOURCE_KEYWORD, jdn, (CalendarDatePart)part);
    if (!text || !*text) { g_free(text); return FALSE; }
    const size_t size = strlen(text);
    if (size >= capacity) { g_free(text); return FALSE; }
    memcpy(buffer, text, size + 1U);
    if (length) *length = size;
    g_free(text);
    return TRUE;
}

static const InfiltratrTemporalResourceMetadata metadata = {
    sizeof(InfiltratrTemporalResourceMetadata), INFILTRATR_TEMPORAL_RESOURCE_METADATA_ABI,
    INFILTRATR_TEMPORAL_RESOURCE_CALENDAR, RESOURCE_ID, INFILTRATR_TEMPORAL_DISPLAY_NAME,
    false, false, false
};
static const InfiltratrTemporalCalendarProvider provider = {
    sizeof(InfiltratrTemporalCalendarProvider), INFILTRATR_TEMPORAL_CALENDAR_PROVIDER_ABI,
    RESOURCE_ID, fields, format, period_start, periods, years
};
#if defined(_WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
EXPORT const InfiltratrTemporalResourceMetadata *infiltratr_temporal_resource_metadata_v1(void);
EXPORT const InfiltratrTemporalCalendarProvider *infiltratr_temporal_calendar_provider_v1(void);
EXPORT const InfiltratrTemporalResourceMetadata *infiltratr_temporal_resource_metadata_v1(void) { return &metadata; }
EXPORT const InfiltratrTemporalCalendarProvider *infiltratr_temporal_calendar_provider_v1(void) { return &provider; }
