#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

int main(int argc, char** argv) {
  doctest::Context context;

  // By default, avoid running benchmarks unless explicitly told to do so.
  context.addFilter("test-case-exclude", "*benchmark*");

  context.applyCommandLine(argc, argv);

  return context.run();
}