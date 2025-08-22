#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/EarthGravitationalModel1996Grid.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <vector>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

struct Egm96TestCase {
public:
  Cartographic cartographicPosition;
  double expectedHeight;

  Egm96TestCase(const Cartographic& position, double height)
      : cartographicPosition(position), expectedHeight(height) {}
};

const std::filesystem::path testFilePath =
    std::filesystem::path(CESIUM_NATIVE_DATA_DIR) / "WW15MGH.DAC";

// Long, Lat values calculated randomly and paired with expected results from
// https://www.unavco.org/software/geodetic-utilities/geoid-height-calculator/geoid-height-calculator.html
const Egm96TestCase testCases[] = {
    Egm96TestCase(
        Cartographic::fromDegrees(135.89012584487307, 11.046411138991914, 0),
        57.79),
    Egm96TestCase(
        Cartographic::fromDegrees(179.78766535213848, -66.77911223257036, 0),
        -57.37),
    Egm96TestCase(
        Cartographic::fromDegrees(281.9977024865146, -81.38156198351201, 0),
        -27.93),
    Egm96TestCase(
        Cartographic::fromDegrees(284.83146919041957, -40.851277013714125, 0),
        10.81),
    Egm96TestCase(
        Cartographic::fromDegrees(194.69062436672687, -70.87693162868418, 0),
        -63.7),
    Egm96TestCase(
        Cartographic::fromDegrees(88.62614642690032, -32.868116497509256, 0),
        -16.45),
    Egm96TestCase(
        Cartographic::fromDegrees(91.20616551626404, 55.05898386587137, 0),
        -38.26),
    Egm96TestCase(
        Cartographic::fromDegrees(77.26636943208759, 11.790177979066698, 0),
        -90.03),
    Egm96TestCase(
        Cartographic::fromDegrees(274.93477305745023, -0.9797391469564616, 0),
        0.73),
    Egm96TestCase(
        Cartographic::fromDegrees(103.42529180264822, -9.133054623669707, 0),
        -12.14),
    Egm96TestCase(
        Cartographic::fromDegrees(124.53620966375256, -77.10236922063635, 0),
        -40.84),
    Egm96TestCase(
        Cartographic::fromDegrees(340.6949744283327, 71.80416601738926, 0),
        56.13),
    Egm96TestCase(
        Cartographic::fromDegrees(256.58178494197625, 29.05072127852661, 0),
        -22.68),
    Egm96TestCase(
        Cartographic::fromDegrees(255.08934167271556, -4.525457356761493, 0),
        -14.03),
    Egm96TestCase(
        Cartographic::fromDegrees(76.00667512450767, -17.683253329717417, 0),
        -41.97),
    Egm96TestCase(
        Cartographic::fromDegrees(86.23997598277842, -70.10334564947195, 0),
        11.84),
    Egm96TestCase(
        Cartographic::fromDegrees(102.06313910716983, 83.28134196702541, 0),
        7.36),
    Egm96TestCase(
        Cartographic::fromDegrees(216.25898282371543, -27.527084126001284, 0),
        -4.14),
    Egm96TestCase(
        Cartographic::fromDegrees(58.5182249193696, 51.31098115052956, 0),
        -14.25),
    Egm96TestCase(
        Cartographic::fromDegrees(330.8502870388745, 39.2404247446803, 0),
        58.02),
    Egm96TestCase(
        Cartographic::fromDegrees(177.4419519702648, 42.39404893293707, 0),
        -10.44),
    Egm96TestCase(
        Cartographic::fromDegrees(110.06737673917638, 82.57103666065765, 0),
        5.75),
    Egm96TestCase(
        Cartographic::fromDegrees(56.90685093006615, 63.5264335297486, 0),
        -1.68),
    Egm96TestCase(
        Cartographic::fromDegrees(266.9690489435701, -58.27419079145019, 0),
        -10.73),
    Egm96TestCase(
        Cartographic::fromDegrees(117.30780499692544, -73.56974180422188, 0),
        -31.02),
    Egm96TestCase(
        Cartographic::fromDegrees(33.16052348335607, -6.0542778852432235, 0),
        -18.28),
    Egm96TestCase(
        Cartographic::fromDegrees(305.22679566909795, -70.42597930709479, 0),
        -0.53),
    Egm96TestCase(
        Cartographic::fromDegrees(68.6870133646387, 2.33895612727828, 0),
        -88.2),
    Egm96TestCase(
        Cartographic::fromDegrees(6.9461874737732465, 57.95503483268874, 0),
        41.76),
    Egm96TestCase(
        Cartographic::fromDegrees(152.24816431673585, -53.85397414122369, 0),
        -20.85),
    Egm96TestCase(
        Cartographic::fromDegrees(213.82755149438987, 68.1242841301565, 0),
        8.51),
    Egm96TestCase(
        Cartographic::fromDegrees(352.01851556423384, -78.67432745187807, 0),
        -6.09),
    Egm96TestCase(
        Cartographic::fromDegrees(18.75098209126253, -4.154279051741511, 0),
        -9.58),
    Egm96TestCase(
        Cartographic::fromDegrees(323.049199598043, -72.38843470090285, 0),
        -2.67),
    Egm96TestCase(
        Cartographic::fromDegrees(140.1466268002612, 21.545270556717682, 0),
        47.96),
    Egm96TestCase(
        Cartographic::fromDegrees(150.55044131405933, 2.145627569983489, 0),
        58.11),
    Egm96TestCase(
        Cartographic::fromDegrees(27.412736310050178, -7.0977340915520415, 0),
        -15.61),
    Egm96TestCase(
        Cartographic::fromDegrees(358.3614938015746, 70.5895724418148, 0),
        50.75),
    Egm96TestCase(
        Cartographic::fromDegrees(244.48155819935246, -18.100608843775944, 0),
        -2.88),
    Egm96TestCase(
        Cartographic::fromDegrees(253.9886632845044, 51.62694479074773, 0),
        -21.71),
    Egm96TestCase(
        Cartographic::fromDegrees(266.1088741204752, -48.05460101900711, 0),
        -4.97),
    Egm96TestCase(
        Cartographic::fromDegrees(55.1493585722661, 28.236862759541495, 0),
        -17.98),
    Egm96TestCase(
        Cartographic::fromDegrees(323.39792289901203, -81.7960770950958, 0),
        -20.34),
    Egm96TestCase(
        Cartographic::fromDegrees(46.27270449946558, 78.09170018252073, 0),
        10.88),
    Egm96TestCase(
        Cartographic::fromDegrees(6.277616646412767, -52.387812595446405, 0),
        25.97),
    Egm96TestCase(
        Cartographic::fromDegrees(326.18341236132915, 4.762653021857375, 0),
        -0.65),
    Egm96TestCase(
        Cartographic::fromDegrees(154.6434159812138, -34.75526648786568, 0),
        15.53),
    Egm96TestCase(
        Cartographic::fromDegrees(316.25406024721343, -52.161238673850676, 0),
        4.29),
    Egm96TestCase(
        Cartographic::fromDegrees(340.23937541216713, -87.35998020843215, 0),
        -23.91),
    Egm96TestCase(
        Cartographic::fromDegrees(359.50010262934694, 1.6307925009477486, 0),
        16.99),
};

// Corner-case testing for bounds of globe
const Egm96TestCase boundsCases[] = {
    Egm96TestCase(Cartographic::fromDegrees(0, 0, 0), 17.16),
    Egm96TestCase(Cartographic::fromDegrees(0, 90, 0), 13.61),
    Egm96TestCase(Cartographic::fromDegrees(0, -90, 0), -29.53),
    Egm96TestCase(Cartographic::fromDegrees(180, 0, 0), 21.15),
    Egm96TestCase(Cartographic::fromDegrees(180, 90, 0), 13.61),
    Egm96TestCase(Cartographic::fromDegrees(180, -90, 0), -29.53),
    Egm96TestCase(Cartographic::fromDegrees(-180, -90, 0), -29.53),
    Egm96TestCase(Cartographic::fromDegrees(-180, 0, 0), 21.15),
    Egm96TestCase(Cartographic::fromDegrees(-180, 90, 0), 13.61),
};

const std::byte zeroByte{0};

TEST_CASE("EarthGravitationalModel1996Grid::fromBuffer") {
  SUBCASE("Loads a valid WW15MGH.DAC from buffer") {
    auto grid =
        EarthGravitationalModel1996Grid::fromBuffer(readFile(testFilePath));
    CHECK(grid.has_value());
  }

  SUBCASE("Fails on too-short buffer") {
    std::vector<std::byte> buffer(4, zeroByte);
    auto grid = EarthGravitationalModel1996Grid::fromBuffer(buffer);
    CHECK(!grid.has_value());
  }

  SUBCASE("Loads an arbitrary correctly-formed buffer") {
    std::vector<std::byte> buffer(3000000, zeroByte);
    auto grid = EarthGravitationalModel1996Grid::fromBuffer(buffer);
    CHECK(grid.has_value());
  }
}

TEST_CASE("EarthGravitationalModel1996Grid::sampleHeight") {
  std::optional<EarthGravitationalModel1996Grid> grid =
      EarthGravitationalModel1996Grid::fromBuffer(readFile(testFilePath));

  SUBCASE("Correct values at bounds") {
    REQUIRE(grid.has_value());

    for (const Egm96TestCase& testCase : boundsCases) {
      const double obtainedValue =
          grid->sampleHeight(testCase.cartographicPosition);
      CHECK(Math::equalsEpsilon(
          testCase.expectedHeight,
          obtainedValue,
          0.0,
          Math::Epsilon2));
    }
  }

  SUBCASE("Calculates correct height values") {
    REQUIRE(grid.has_value());

    for (const Egm96TestCase& testCase : testCases) {
      const double obtainedValue =
          grid->sampleHeight(testCase.cartographicPosition);
      const bool equals = (Math::equalsEpsilon(
          testCase.expectedHeight,
          obtainedValue,
          0.0,
          Math::Epsilon2));
      CHECK(equals);
    }
  }
}
