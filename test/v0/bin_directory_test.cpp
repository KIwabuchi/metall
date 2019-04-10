// Copyright 2019 Lawrence Livermore National Security, LLC and other Metall Project Developers.
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#include "gtest/gtest.h"
#include <memory>
#include <metall/v0/kernel/bin_directory.hpp>
#include <metall/v0/kernel/bin_number_manager.hpp>
#include <metall/metall.hpp>

namespace {
using bin_no_mngr = metall::v0::kernel::bin_number_manager<metall::manager::chunk_size(), 1ULL << 48>;
constexpr int num_small_bins = bin_no_mngr::to_bin_no(metall::manager::chunk_size() / 2) + 1;
using directory_type = metall::v0::kernel::bin_directory<num_small_bins,
                                                         metall::manager::chunk_number_type,
                                                         std::allocator<char>>;

TEST(BinDirectoryTest, Front) {
  std::allocator<char> allocator;
  directory_type obj(allocator);

  obj.insert(0, 1);
  ASSERT_EQ(obj.front(0), 1);

  obj.insert(0, 2);
#ifdef USE_SPACE_AWARE_BIN
  ASSERT_EQ(obj.front(0), 1);
#else
  ASSERT_EQ(obj.front(0), 2);
#endif

  obj.insert(num_small_bins - 1, 3);
  ASSERT_EQ(obj.front(num_small_bins - 1), 3);

  obj.insert(num_small_bins - 1, 4);
#ifdef USE_SPACE_AWARE_BIN
  ASSERT_EQ(obj.front(num_small_bins - 1), 3);
#else
  ASSERT_EQ(obj.front(num_small_bins - 1), 4);
#endif
}

TEST(BinDirectoryTest, Empty) {
  std::allocator<char> allocator;
  directory_type obj(allocator);

  ASSERT_TRUE(obj.empty(0));
  obj.insert(0, 1);
  ASSERT_FALSE(obj.empty(0));

  ASSERT_TRUE(obj.empty(num_small_bins - 1));
  obj.insert(num_small_bins - 1, 1);
  ASSERT_FALSE(obj.empty(num_small_bins - 1));
}

TEST(BinDirectoryTest, Pop) {
  std::allocator<char> allocator;
  directory_type obj(allocator);

  ASSERT_TRUE(obj.empty(0));
  obj.insert(0, 1);
  ASSERT_FALSE(obj.empty(0));
  obj.pop(0);
  ASSERT_TRUE(obj.empty(0));

  ASSERT_TRUE(obj.empty(num_small_bins - 1));
  obj.insert(num_small_bins - 1, 1);
  ASSERT_FALSE(obj.empty(num_small_bins - 1));
  obj.pop(num_small_bins - 1);
  ASSERT_TRUE(obj.empty(num_small_bins - 1));
}

TEST(BinDirectoryTest, Erase) {
  std::allocator<char> allocator;
  directory_type obj(allocator);

  obj.insert(0, 1);
  ASSERT_TRUE(obj.erase(0, 1));
  ASSERT_FALSE(obj.erase(0, 1));

  obj.insert(num_small_bins - 1, 1);
  ASSERT_TRUE(obj.erase(num_small_bins - 1, 1));
  ASSERT_FALSE(obj.erase(num_small_bins - 1, 1));

}

TEST(BinDirectoryTest, Serialize) {
  std::allocator<char> allocator;
  directory_type obj(allocator);

  obj.insert(0, 1);
  obj.insert(0, 2);
  obj.insert(num_small_bins - 1, 3);
  obj.insert(num_small_bins - 1, 4);

  ASSERT_TRUE(obj.serialize("/tmp/bin_directory_test_file"));
}

TEST(BinDirectoryTest, Deserialize) {
  {
    std::allocator<char> allocator;
    directory_type obj(allocator);

    obj.insert(0, 1);
    obj.insert(0, 2);
    obj.insert(num_small_bins - 1, 3);
    obj.insert(num_small_bins - 1, 4);

    obj.serialize("/tmp/bin_directory_test_file");
  }

  {
    std::allocator<char> allocator;
    directory_type obj(allocator);
    ASSERT_TRUE(obj.deserialize("/tmp/bin_directory_test_file"));

#ifdef USE_SPACE_AWARE_BIN
    ASSERT_EQ(obj.front(0), 1);
    obj.pop(0);
    ASSERT_EQ(obj.front(0), 2);
#else
    ASSERT_EQ(obj.front(0), 2);
    obj.pop(0);
    ASSERT_EQ(obj.front(0), 1);
#endif

#ifdef USE_SPACE_AWARE_BIN
    ASSERT_EQ(obj.front(num_small_bins - 1), 3);
    obj.pop(num_small_bins - 1);
    ASSERT_EQ(obj.front(num_small_bins - 1), 4);
#else
    ASSERT_EQ(obj.front(num_small_bins - 1), 4);
    obj.pop(num_small_bins - 1);
    ASSERT_EQ(obj.front(num_small_bins - 1), 3);
#endif

  }
}

}