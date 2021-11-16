#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include "trace_categories.h"

// #ifdef _MSC_VER
// #pragma warning(push)
// #pragma warning(disable : 4127)

// #define NOMINMAX
// #define WIN32_LEAN_AND_MEAN
// #endif

// #include <perfetto.h>

// #ifdef _MSC_VER
// #pragma warning(pop)
// #endif

// // The set of track event categories that the example is using.
// PERFETTO_DEFINE_CATEGORIES(
//     perfetto::Category("rendering")
//         .SetDescription("Rendering and graphics events"),
//     perfetto::Category("network.debug")
//         .SetTags("debug")
//         .SetDescription("Verbose network events"),
//     perfetto::Category("audio.latency")
//         .SetTags("verbose")
//         .SetDescription("Detailed audio latency metrics"));

// PERFETTO_TRACK_EVENT_STATIC_STORAGE();

int main(int argc, char* argv[]) {
  perfetto::TracingInitArgs args;
  // The backends determine where trace events are recorded. For this example we
  // are going to use the in-process tracing service, which only includes in-app
  // events.
  args.backends = perfetto::kInProcessBackend;

  perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();

  int result = Catch::Session().run(argc, argv);

  // global clean-up...

  return result;
}
