#include <numbers>

template <typename F>
constexpr F degrees_to_radians(const F degrees) noexcept {
    return degrees * (std::numbers::pi_v<F> / static_cast<F>(180.));
}
