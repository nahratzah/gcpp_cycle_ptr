#include "cycle_ptr.h"
#include "UnitTest++/UnitTest++.h"
#include "create_destroy_check.h"

struct csc_container {
  constexpr csc_container(bool* destroyed) noexcept
  : data(destroyed)
  {}

  create_destroy_check data;
  int foo = 4;
};

TEST(gptr_constructor) {
  cpp2::gc gc;

  CHECK(gc.make<int>(4) != nullptr);
}

TEST(destructor) {
  cpp2::gc gc;

  bool destroyed = false;
  cpp2::gc::gptr<create_destroy_check> ptr =
      gc.make<create_destroy_check>(&destroyed);
  REQUIRE CHECK(ptr != nullptr);

  CHECK(!destroyed);
  ptr = nullptr;
  CHECK(destroyed);
}

TEST(share) {
  cpp2::gc gc;

  bool destroyed = false;
  cpp2::gc::gptr<create_destroy_check> ptr_1 =
      gc.make<create_destroy_check>(&destroyed);
  cpp2::gc::gptr<create_destroy_check> ptr_2 = ptr_1;
  REQUIRE CHECK(ptr_1 != nullptr);
  REQUIRE CHECK(ptr_2 != nullptr);

  CHECK(!destroyed);
  ptr_1 = nullptr;
  CHECK(!destroyed);
  ptr_2 = nullptr;
  CHECK(destroyed);
}

TEST(alias) {
  cpp2::gc gc;

  bool destroyed = false;
  cpp2::gc::gptr<csc_container> ptr_1 =
      gc.make<csc_container>(&destroyed);
  REQUIRE CHECK(ptr_1 != nullptr);
  auto alias = cpp2::gc::gptr<int>(ptr_1, &ptr_1->foo);
  REQUIRE CHECK(alias != nullptr);

  const int*const foo_addr = &ptr_1->foo;

  CHECK_EQUAL(foo_addr, alias.get());
  ptr_1 = nullptr;
  CHECK_EQUAL(foo_addr, alias.get());
  CHECK(!destroyed);

  alias = nullptr;
  CHECK(destroyed);
}
