add_executable(test_kernel
    test_kernel.cpp
)

target_link_libraries(test_kernel
  PRIVATE
    core_interface
    bitcoinkernel
    Boost::headers
)

add_windows_application_manifest(test_kernel)

add_test(NAME test_kernel COMMAND test_kernel)

add_executable(bench_kernel_logging
    bench_kernel_logging.cpp
)

target_link_libraries(bench_kernel_logging
  PRIVATE
    core_interface
    bitcoinkernel
)
