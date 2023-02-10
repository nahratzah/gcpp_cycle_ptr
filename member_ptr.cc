#include "cycle_ptr.h"
#include "UnitTest++/UnitTest++.h"
#include <vector>
#include "create_destroy_check.h"

class owner
: public create_destroy_check
{
 public:
  owner(bool* destroyed_owner, bool* destroyed_target)
  : create_destroy_check(destroyed_owner),
    target(cpp2::gc().make<create_destroy_check>(destroyed_target))
  {}

  explicit owner(bool* destroyed_owner)
  : create_destroy_check(destroyed_owner)
  {}

  cpp2::gc::member_ptr<create_destroy_check> target;
};

class owner_of_collection
: public cycle_ptr::cycle_base
{
 public:
  using vector_type = std::vector<
      cpp2::gc::member_ptr<create_destroy_check>,
      cpp2::gc::allocator<cpp2::gc::member_ptr<create_destroy_check>>>;

  owner_of_collection()
  : data(vector_type::allocator_type(*this))
  {}

  owner_of_collection(const owner_of_collection& y)
  : data(y.data, vector_type::allocator_type(*this))
  {}

  owner_of_collection(owner_of_collection&& y)
  : data(std::move(y.data), vector_type::allocator_type(*this))
  {}

  auto operator=(const owner_of_collection& y)
  -> owner_of_collection& {
    data = y.data;
    return *this;
  }

  auto operator=(owner_of_collection&& y)
  -> owner_of_collection& {
    data = std::move(y.data);
    return *this;
  }

  template<typename Iter>
  owner_of_collection(Iter b, Iter e)
  : data(b, e, vector_type::allocator_type(*this))
  {}

  vector_type data;
};

TEST(member_ptr_constructor) {
  cpp2::gc gc;

  bool owner_destroyed = false;
  bool target_destroyed = false;
  CHECK(gc.make<owner>(&owner_destroyed, &target_destroyed)->target != nullptr);

  CHECK(owner_destroyed);
  CHECK(target_destroyed);
}

TEST(assignment) {
  cpp2::gc gc;

  bool owner_destroyed = false;
  bool target_destroyed = false;
  cpp2::gc::gptr<owner> ptr_1 = gc.make<owner>(&owner_destroyed);

  ptr_1->target = gc.make<create_destroy_check>(&target_destroyed);
  CHECK(ptr_1->target != nullptr);
  CHECK(!owner_destroyed);
  CHECK(!target_destroyed);

  ptr_1 = nullptr;
  CHECK(owner_destroyed);
  CHECK(target_destroyed);
}

TEST(null_pointee) {
  cpp2::gc gc;

  bool owner_destroyed = false;
  cpp2::gc::gptr<owner> ptr_1 = gc.make<owner>(&owner_destroyed);

  REQUIRE CHECK(ptr_1->target == nullptr);
  CHECK(!owner_destroyed);

  ptr_1 = nullptr;
  CHECK(owner_destroyed);
}

TEST(self_reference) {
  cpp2::gc gc;

  bool destroyed = false;
  cpp2::gc::gptr<owner> ptr = gc.make<owner>(&destroyed);
  ptr->target = ptr;

  CHECK(!destroyed);
  ptr = nullptr;
  CHECK(destroyed);
}

TEST(cycle) {
  cpp2::gc gc;

  bool first_destroyed = false;
  bool second_destroyed = false;
  cpp2::gc::gptr<owner> ptr_1 = gc.make<owner>(&first_destroyed);
  cpp2::gc::gptr<owner> ptr_2 = gc.make<owner>(&second_destroyed);
  ptr_1->target = ptr_2;
  ptr_2->target = ptr_1;

  REQUIRE CHECK_EQUAL(ptr_2, ptr_1->target);
  REQUIRE CHECK_EQUAL(ptr_1, ptr_2->target);

  CHECK(!first_destroyed);
  CHECK(!second_destroyed);

  ptr_1 = nullptr;
  CHECK(!first_destroyed);
  CHECK(!second_destroyed);

  ptr_2 = nullptr;
  CHECK(first_destroyed);
  CHECK(second_destroyed);
}

TEST(move_seq) {
  cpp2::gc gc;

  std::vector<cpp2::gc::gptr<create_destroy_check>> pointers;
  std::generate_n(
      std::back_inserter(pointers),
      10'000,
      [&gc]() {
        return gc.make<create_destroy_check>();
      });

  auto ooc = gc.make<owner_of_collection>(pointers.cbegin(), pointers.cend());
}

TEST(expired_can_assign) {
  cpp2::gc gc;

  struct testclass {
    bool* td_ptr;
    cpp2::gc::member_ptr<create_destroy_check> ptr;

    explicit testclass(bool* td_ptr) : td_ptr(td_ptr) {}
    ~testclass() noexcept(false) { // Test runs during the destructor.
      cpp2::gc gc;
      ptr = gc.make<create_destroy_check>(td_ptr);
      CHECK_EQUAL(nullptr, ptr);
    }
  };

  bool destroyed = false;
  auto tc = gc.make<testclass>(&destroyed);
  REQUIRE CHECK(tc != nullptr);
  tc.reset();
  REQUIRE CHECK(tc == nullptr);

  CHECK(destroyed);
}

TEST(expired_can_reset) {
  cpp2::gc gc;

  struct testclass {
    cpp2::gc::member_ptr<int> ptr;

    testclass()
    : ptr(cpp2::gc().make<int>())
    {}

    ~testclass() noexcept(false) { // Test runs during the destructor.
      CHECK_EQUAL(nullptr, ptr);
      ptr.reset(); // We don't guarantee a specific moment of destruction.
      CHECK_EQUAL(nullptr, ptr);
    }
  };

  auto tc = gc.make<testclass>();
  REQUIRE CHECK(tc != nullptr);
  tc.reset();
  REQUIRE CHECK(tc == nullptr);
}

TEST(expired_can_create_gptr_but_wont_resurrect) {
  cpp2::gc gc;

  struct testclass {
    cpp2::gc::gptr<int>& gptr;
    cpp2::gc::member_ptr<int> ptr;

    testclass(cpp2::gc::gptr<int>& gptr)
    : gptr(gptr),
      ptr(cpp2::gc().make<int>())
    {}

    ~testclass() { // Test runs during the destructor.
      gptr = ptr; // Because `ptr` is expired, gptr will be set to null.
    }
  };

  cpp2::gc::gptr<int> gptr = gc.make<int>(42);
  auto tc = gc.make<testclass>(gptr);
  REQUIRE CHECK(tc != nullptr);
  tc.reset();
  REQUIRE CHECK(tc == nullptr);
  CHECK_EQUAL(nullptr, gptr);
}
