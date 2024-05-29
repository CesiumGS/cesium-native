#pragma once

#include <glm/geometric.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <random>

namespace CesiumNativeTests {

// Produce a random N-dimensional unit vector. Use a constant seed in order to
// get a repeatable stream of vectors that can then be debugged!
template <typename Vec> struct RandomUnitVectorGenerator {
  using value_type = typename Vec::value_type;
  std::mt19937 gen;
  std::uniform_real_distribution<value_type> dis;

  RandomUnitVectorGenerator()
      : dis(std::nextafter(static_cast<value_type>(-1), value_type()),
            static_cast<value_type>(1)) {
    gen.seed(42);
  }

  Vec operator()() {
    Vec result(0);
    value_type length2 = 0;
    // If we don't discard values that fall outside the unit sphere, then the
    // points are biased towards the corners of the unit cube.
    do {
      for (glm::length_t i = 0; i < Vec::length(); ++i) {
        result[i] = dis(gen);
      }
      length2 = dot(result, result);
    } while (length2 > 1 || length2 == 0);
    return result / std::sqrt(length2);
  }
};

template <typename T> struct RandomQuaternionGenerator {
  RandomUnitVectorGenerator<glm::vec<4, T>> vecGenerator;

  glm::qua<T> operator()() {
    glm::vec<4, T> vec = vecGenerator();
    return glm::qua<T>(vec.w, vec.x, vec.y, vec.z);
  }
};
} // namespace CesiumNativeTests
