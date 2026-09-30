// Checks wordclock::palette() finds every palette colour by name, and that
// unknown names (including prefixes of real ones) are black. Prints the names
// so run.sh can compare them with colour_select.yaml.
#include <cstdio>
#include <initializer_list>

#include "../wordclock.h"

static bool same(wordclock::Rgb a, wordclock::Rgb b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

int main() {
  int failures = 0;
  for (const auto &colour : wordclock::kPalette) {
    if (!same(wordclock::palette(colour.name), colour.rgb)) {
      std::fprintf(stderr, "palette(\"%s\") is wrong\n", colour.name);
      failures++;
    }
    std::printf("%s\n", colour.name);
  }
  for (const char *name : {"", "Re", "Redd", "red", "Nope"}) {
    if (!same(wordclock::palette(name), {0, 0, 0})) {
      std::fprintf(stderr, "palette(\"%s\") should be black\n", name);
      failures++;
    }
  }
  return failures ? 1 : 0;
}
