// Word clock face logic for the GurgleApps 8x8 colour word clock.
//
// Pure C++ with no ESPHome dependencies, so it can be unit tested on a host
// (see test/). Ported from build_time_word_data() in the original MicroPython
// firmware: https://github.com/gurgleapps/Gurgle-Apps-Word-Clock
//
// Layout: each word is an 8-byte mask, one byte per row (row 0 = first LED
// row), with column 0 in bit 7. LED index = row * 8 + col.
#pragma once

#include <cstdint>

namespace wordclock {

static constexpr int kRows = 8;
static constexpr int kCols = 8;
static constexpr int kNumLeds = kRows * kCols;

using Mask = uint8_t[kRows];

static constexpr Mask kPast = {0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr Mask kTo = {0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};

// Index 0 is twelve o'clock.
static constexpr Mask kHours[12] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x00},  // 12
    {0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00},  // 1
    {0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x40, 0x00},  // 2
    {0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00},  // 3
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00},  // 4
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00},  // 5
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0},  // 6
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f},  // 7
    {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00},  // 8
    {0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00},  // 9
    {0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00},  // 10
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00},  // 11
};

// Index n is n * 5 minutes past/to; index 0 (o'clock) is blank.
static constexpr Mask kMinutes[7] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // o'clock
    {0x00, 0xd4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // five
    {0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // ten
    {0x00, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // quarter
    {0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // twenty
    {0x3f, 0xd4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // twenty five
    {0xc0, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00},  // half
};

// Shown until the clock has a valid time.
static constexpr Mask kWifi = {0x3c, 0x42, 0x99, 0xa5, 0x24, 0x00, 0x18, 0x18};

inline bool mask_bit(const uint8_t *mask, int led) {
  return mask[led / kCols] & (0x80 >> (led % kCols));
}

// Colour wheel, 0-255 cycles red -> green -> blue -> red.
struct Rgb {
  uint8_t r, g, b;
};

inline Rgb wheel(uint8_t pos) {
  if (pos < 85)
    return {static_cast<uint8_t>(255 - pos * 3), static_cast<uint8_t>(pos * 3), 0};
  if (pos < 170) {
    pos -= 85;
    return {0, static_cast<uint8_t>(255 - pos * 3), static_cast<uint8_t>(pos * 3)};
  }
  pos -= 170;
  return {static_cast<uint8_t>(pos * 3), 0, static_cast<uint8_t>(255 - pos * 3)};
}

// Rainbow spread across the whole face, as in the original "rainbow" mode.
inline Rgb rainbow(int led, uint8_t offset = 0) {
  return wheel(static_cast<uint8_t>(led * 256 / kNumLeds + offset));
}

// What a given LED is showing, used for per-word colouring.
enum class Part : uint8_t { NONE = 0, PAST_TO, HOUR, MINUTE };

struct Face {
  uint8_t past_to[kRows] = {};
  uint8_t hour[kRows] = {};
  uint8_t minute[kRows] = {};

  // Later words win where masks overlap, matching the original firmware's
  // colour-per-word ordering (past/to, then hour, then minute).
  Part part(int led) const {
    if (mask_bit(minute, led))
      return Part::MINUTE;
    if (mask_bit(hour, led))
      return Part::HOUR;
    if (mask_bit(past_to, led))
      return Part::PAST_TO;
    return Part::NONE;
  }

  bool lit(int led) const { return part(led) != Part::NONE; }
};

// hour: 0-23, minute: 0-59 (local time).
inline Face compose(int hour, int minute) {
  Face face;
  // Round to the nearest five minutes; 58-59 round up to the next hour.
  int rounded = (minute + 2) / 5 * 5;
  const uint8_t *link = nullptr;
  if (rounded > 30) {
    hour += 1;
    if (rounded < 60)
      link = kTo;
  } else if (rounded > 0) {
    link = kPast;
  }
  int words = rounded > 30 ? (60 - rounded) / 5 : rounded / 5;
  const uint8_t *hour_mask = kHours[hour % 12];
  const uint8_t *minute_mask = kMinutes[words];
  for (int r = 0; r < kRows; r++) {
    face.past_to[r] = link ? link[r] : 0;
    face.hour[r] = hour_mask[r];
    face.minute[r] = minute_mask[r];
  }
  return face;
}

}  // namespace wordclock
