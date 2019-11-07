// Copyright 2019 Lawrence Livermore National Security, LLC and other Metall Project Developers.
// See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include <metall/metall.hpp>
#include <metall/detail/utility/time.hpp>
#include <metall/detail/utility/file_clone.hpp>
#include "../data_structure/multithread_adjacency_list.hpp"
#include "bench_driver.hpp"

using namespace adjacency_list_bench;

using key_type = uint64_t;
using value_type = uint64_t;

using adjacency_list_type =  data_structure::multithread_adjacency_list<key_type,
                                                                        value_type,
                                                                        typename metall::manager::allocator_type<std::byte>>;

void normal_copy(const std::string &source_path, const std::string &destination_path) {
  std::string rm_command("cp -R " + source_path + " " + destination_path);
  std::system(rm_command.c_str());
  if (!metall::detail::utility::fsync(destination_path)) {
    std::abort();
  }
}

void reflink_copy(const std::string &source_path, const std::string &destination_path) {
#if defined(__linux__)
  std::string rm_command("cp --reflink=auto -R " + source_path + " " + destination_path);
  std::system(rm_command.c_str());
#elif defined(__APPLE__)
  std::string rm_command("cp -cR " + source_path + " " + destination_path);
  std::system(rm_command.c_str());
#endif

  if (!metall::detail::utility::fsync(destination_path)) {
    std::abort();
  }
}

int main(int argc, char *argv[]) {

  bench_options option;
  if (!parse_options(argc, argv, &option)) {
    std::abort();
  }

  if (option.segment_file_name_list.empty()) {
    std::cerr << "Segment file name is required" << std::endl;
    std::abort();
  }

  {
    metall::manager manager(metall::create_only, option.segment_file_name_list[0].c_str(), option.segment_size);

    // This function is called after inserting each chunk
    std::size_t snapshot_num = 0;
    auto snapshot_func = [&option, &manager, &snapshot_num]() {
      manager.sync();

      std::stringstream snapshot_id;
      snapshot_id << std::setw(4) << std::setfill('0') << std::to_string(snapshot_num);
      {
        const auto snapshot_dir = option.segment_file_name_list[0] + "-normal-snapshot-" + snapshot_id.str();
        const auto start = util::elapsed_time_sec();
        normal_copy(option.segment_file_name_list[0], snapshot_dir);
        const auto elapsed_time = util::elapsed_time_sec(start);
        std::cout << "Normal copy took (s)\t" << elapsed_time << std::endl;
      }

      {
        const auto snapshot_dir = option.segment_file_name_list[0] + "-reflink-snapshot-" + snapshot_id.str();
        const auto start = util::elapsed_time_sec();
        reflink_copy(option.segment_file_name_list[0], snapshot_dir);
        const auto elapsed_time = util::elapsed_time_sec(start);
        std::cout << "reflink copy took (s)\t" << elapsed_time << std::endl;
      }

      ++snapshot_num;
    };

    auto adj_list = manager.construct<adjacency_list_type>(option.adj_list_key_name.c_str())(snapshot_func,
                                                                                             manager.get_allocator<>());

    run_bench(option, single_numa_bench, adj_list);
  }

  return 0;
}