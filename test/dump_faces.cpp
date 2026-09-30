// Prints the face for every minute of the day, one line per minute:
// "HH:MM <64 digits>" where each digit is the wordclock::Part of that LED.
// Then one "rainbow" line per LED with the static rainbow colour.
// Then "rain <scenario> <frame> <64 rrggbb>" lines for matrix rain scenarios
// driven by a fixed pseudo-random sequence (must match reference.py).
#include <cstdio>

#include "../wordclock.h"

namespace {

struct RainScenario {
  bool white_head, rain_over_words;
  int trail_length, spawn_rate, brightness_level;  // level 0-15, as stock
  wordclock::Rgb background;
};

const RainScenario kRainScenarios[] = {
    {false, false, 4, 18, 2, {0, 255, 0}},   // stock defaults
    {true, true, 4, 18, 2, {0, 255, 0}},     //
    {true, false, 8, 60, 15, {0, 200, 255}},  // brightness cap active
    {false, true, 1, 100, 7, {255, 64, 0}},   // brightness cap active
    {true, true, 6, 5, 3, {0, 255, 0}},       // at the cap
};
constexpr int kRainFrames = 200;
constexpr int kTimeBrightnessCap = 3;
const wordclock::Rgb kHour = {255, 255, 0}, kMinute = {255, 0, 0}, kPastTo = {255, 128, 0};

uint32_t lcg_state = 1;
uint32_t lcg() {
  lcg_state = (lcg_state * 1103515245u + 12345u) & 0x7fffffffu;
  return lcg_state;
}

}  // namespace

int main() {
  for (int h = 0; h < 24; h++) {
    for (int m = 0; m < 60; m++) {
      wordclock::Face face = wordclock::compose(h, m);
      std::printf("%02d:%02d ", h, m);
      for (int led = 0; led < wordclock::kNumLeds; led++)
        std::printf("%d", static_cast<int>(face.part(led)));
      std::printf("\n");
    }
  }
  for (int led = 0; led < wordclock::kNumLeds; led++) {
    wordclock::Rgb c = wordclock::rainbow(led);
    std::printf("rainbow %d %d %d %d\n", led, c.r, c.g, c.b);
  }
  int index = 0;
  for (const auto &sc : kRainScenarios) {
    wordclock::MatrixRain rain;
    lcg_state = 1;
    int num = 1, den = 1;
    if (sc.brightness_level > kTimeBrightnessCap) {
      num = kTimeBrightnessCap + 1;
      den = sc.brightness_level + 1;
    }
    for (int frame = 0; frame < kRainFrames; frame++) {
      int t = index * 97 + frame * 3;
      wordclock::Face face = wordclock::compose(t / 60 % 24, t % 60);
      rain.advance([] { return lcg() % 100; }, sc.spawn_rate, sc.trail_length);
      std::printf("rain %d %d ", index, frame);
      for (int led = 0; led < wordclock::kNumLeds; led++) {
        wordclock::Rgb c = rain.render(led, face, sc.background, kHour, kMinute, kPastTo, sc.white_head,
                                       sc.rain_over_words, num, den);
        std::printf("%02x%02x%02x", c.r, c.g, c.b);
      }
      std::printf("\n");
    }
    index++;
  }
  return 0;
}
