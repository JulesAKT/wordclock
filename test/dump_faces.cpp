// Prints the face for every minute of the day, one line per minute:
// "HH:MM <64 digits>" where each digit is the wordclock::Part of that LED.
// Then one "rainbow" line per LED with the static rainbow colour.
#include <cstdio>

#include "../wordclock.h"

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
  return 0;
}
