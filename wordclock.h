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

// Named colours offered by the colour settings (colour_select.yaml).
struct NamedColour {
  const char *name;
  Rgb rgb;
};

static constexpr NamedColour kPalette[] = {
    {"Red", {255, 0, 0}},     {"Orange", {255, 128, 0}}, {"Yellow", {255, 255, 0}}, {"Green", {0, 255, 0}},
    {"Cyan", {0, 255, 255}},  {"Blue", {0, 0, 255}},     {"Purple", {128, 0, 255}}, {"Pink", {255, 0, 128}},
    {"White", {255, 255, 255}}, {"Off", {0, 0, 0}},
};

// Unknown names are black.
inline Rgb palette(const char *name) {
  for (const auto &colour : kPalette) {
    const char *a = colour.name, *b = name;
    while (*a && *a == *b)
      a++, b++;
    if (*a == *b)
      return colour.rgb;
  }
  return {0, 0, 0};
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

// "Matrix rain": columns of falling, fading trails behind the time words.
// Ported from matrix_rain.py in the original firmware.
struct MatrixRain {
  static constexpr int kFadeStep = 36;
  static constexpr int kHeadTrailStep = 45;
  static constexpr int kWhiteHeadThreshold = 235;

  struct Column {
    bool active = false;
    int head = 0;
    int trail_length = 0;
  };

  uint8_t intensity[kRows][kCols] = {};
  Column columns[kCols];

  // One animation step. rand100() returns a random integer 0-99; spawn_rate
  // is the percentage chance per step of an idle column starting a trail.
  template<typename Rand> void advance(Rand rand100, int spawn_rate, int trail_length) {
    for (auto &row : intensity)
      for (auto &value : row)
        value = value > kFadeStep ? value - kFadeStep : 0;

    for (int col = 0; col < kCols; col++) {
      Column &column = columns[col];
      if (!column.active) {
        if (static_cast<int>(rand100()) < spawn_rate) {
          column.active = true;
          column.head = 0;
          column.trail_length = trail_length;
        }
        continue;
      }
      for (int offset = 0; offset < column.trail_length; offset++) {
        int row = column.head - offset;
        int value = 255 - offset * kHeadTrailStep;
        if (row >= 0 && row < kRows && value > intensity[row][col])
          intensity[row][col] = value;
      }
      column.head++;
      if (column.head - column.trail_length > kRows - 1)
        column.active = false;
    }
  }

  Rgb rain_colour(int led, Rgb background, bool white_head) const {
    int value = intensity[led / kCols][led % kCols];
    if (value <= 0)
      return {0, 0, 0};
    if (white_head && value >= kWhiteHeadThreshold)
      return {255, 255, 255};
    return {static_cast<uint8_t>(background.r * value / 255), static_cast<uint8_t>(background.g * value / 255),
            static_cast<uint8_t>(background.b * value / 255)};
  }

  // Colour of one LED: the time words over the rain. With rain_over_words,
  // rain passing through a word brightens it instead of being hidden.
  // Word colours are scaled by word_scale_num / word_scale_den (the original's
  // brightness cap, keeping words from outshining the rain).
  Rgb render(int led, const Face &face, Rgb background, Rgb hour, Rgb minute, Rgb past_to, bool white_head,
             bool rain_over_words, int word_scale_num = 1, int word_scale_den = 1) const {
    Rgb rain = rain_colour(led, background, white_head);
    Rgb word;
    switch (face.part(led)) {
      case Part::HOUR:
        word = hour;
        break;
      case Part::MINUTE:
        word = minute;
        break;
      case Part::PAST_TO:
        word = past_to;
        break;
      default:
        return rain;
    }
    bool raining = rain.r || rain.g || rain.b;
    if (rain_over_words && raining)
      word = {word.r > rain.r ? word.r : rain.r, word.g > rain.g ? word.g : rain.g,
              word.b > rain.b ? word.b : rain.b};
    return {static_cast<uint8_t>(word.r * word_scale_num / word_scale_den),
            static_cast<uint8_t>(word.g * word_scale_num / word_scale_den),
            static_cast<uint8_t>(word.b * word_scale_num / word_scale_den)};
  }
};

}  // namespace wordclock
