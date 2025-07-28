#include <random>

#include <raytracer/config.hpp>

template <typename F>
F urand01() noexcept {
    const auto get_rng_seed = []{
        if constexpr (fixed_rng_seed.has_value()) {
            return fixed_rng_seed.value();
        } else {
            std::random_device rd;
            return rd();
        }
    };

    thread_local std::minstd_rand engine(get_rng_seed());

    return std::generate_canonical<F, std::numeric_limits<F>::digits>(engine);
}

