#include <iostream>
#include <cstdlib>

class create_destroy_check {
 public:
  constexpr create_destroy_check() noexcept = default;

  constexpr create_destroy_check(bool* destroyed) noexcept
  : destroyed(destroyed)
  {}

  ~create_destroy_check() {
    if (destroyed != nullptr) {
      if (*destroyed) { // Already destroyed!
        std::cerr << "Double destroyed!\n";
        std::abort();
      }

      *destroyed = true;
    }
  }

 private:
  bool* destroyed = nullptr;
};
