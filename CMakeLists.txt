cmake_minimum_required(VERSION 3.22)

project(AIR VERSION 0.10.0 LANGUAGES CXX)

option(AIR_BUILD_TESTS "Build AIR tests" ON)
option(AIR_ENABLE_CUDA "Build the NVIDIA CUDA executor" OFF)
option(AIR_ENABLE_NVTX "Enable opt-in NVTX ranges for CUDA profiling builds" OFF)
option(AIR_BUILD_RESEARCH "Compatibility switch for AIR research workflows; no separate research binaries are built" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
include(CheckIncludeFileCXX)
include(CMakePackageConfigHelpers)
check_include_file_cxx("boost/beast.hpp" AIR_HAVE_BOOST_BEAST)
check_include_file_cxx("boost/json.hpp" AIR_HAVE_BOOST_JSON)
if(NOT AIR_HAVE_BOOST_BEAST OR NOT AIR_HAVE_BOOST_JSON)
    message(FATAL_ERROR "AIR server requires Boost.Beast and Boost.JSON headers")
endif()
pkg_check_modules(PCRE2 REQUIRED IMPORTED_TARGET libpcre2-8)

function(air_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow>)
    endif()
endfunction()

add_library(air_core
    src/model/model.cpp
    src/model/storage.cpp
    src/model/tokenizer.cpp
    src/model/architecture_adapter.cpp
    src/model/prepared_model.cpp
    src/model/qwen2_contract.cpp
    src/model/flux2_klein_semantics.cpp
    src/runtime/execution.cpp
    src/runtime/decision.cpp
    src/runtime/hardware.cpp
    src/runtime/hardware_json.cpp
    src/runtime/observation.cpp
    src/runtime/backend.cpp
    src/runtime/scheduler.cpp
    src/runtime/sequence_state_store.cpp
    src/runtime/runtime.cpp
    src/runtime/workload.cpp
    src/runtime/work_unit.cpp
    src/runtime/invocation.cpp
    src/runtime/resource.cpp
    src/runtime/flux2_klein_plan.cpp
    src/runtime/verification.cpp
    src/runtime/manifest.cpp
    src/runtime/qualification.cpp
    src/boost_json.cpp
    src/runtime/sampling.cpp
    src/runtime/generation.cpp
    src/runtime/serving.cpp
    src/benchmark/benchmark.cpp
    src/reference/tensor_reader.cpp
    src/reference/reference_executor.cpp
    src/formats/gguf/gguf_format.cpp
    src/formats/gguf/gguf_reader.cpp
)
add_library(AIR::core ALIAS air_core)
set_target_properties(air_core PROPERTIES EXPORT_NAME core)
target_compile_features(air_core PUBLIC cxx_std_20)

target_include_directories(air_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(air_core PRIVATE PkgConfig::PCRE2 Threads::Threads)
target_compile_definitions(air_core
    PUBLIC
        AIR_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        AIR_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        AIR_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    PRIVATE
        PCRE2_CODE_UNIT_WIDTH=8
)

air_enable_warnings(air_core)

if(AIR_ENABLE_CUDA)
    include(CheckLanguage)
    check_language(CUDA)
    if(NOT CMAKE_CUDA_COMPILER)
        message(FATAL_ERROR "AIR_ENABLE_CUDA=ON but no CUDA compiler was found")
    endif()
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)

    add_library(air_cuda src/cuda/cuda_backend.cu)
    target_link_libraries(air_cuda PUBLIC AIR::core PRIVATE CUDA::cudart CUDA::cublas)
    target_include_directories(air_cuda PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(air_cuda PUBLIC AIR_HAS_CUDA=1)
    if(AIR_ENABLE_NVTX)
        find_path(AIR_NVTX_INCLUDE_DIR nvtx3/nvToolsExt.h HINTS ${CUDAToolkit_INCLUDE_DIRS})
        if(NOT AIR_NVTX_INCLUDE_DIR)
            message(FATAL_ERROR "AIR_ENABLE_NVTX=ON but NVTX v3 headers were not found in the CUDA toolkit")
        endif()
        target_include_directories(air_cuda PRIVATE ${AIR_NVTX_INCLUDE_DIR})
        target_compile_definitions(air_cuda PRIVATE AIR_HAS_NVTX=1)
        if(UNIX)
            target_link_libraries(air_cuda PRIVATE ${CMAKE_DL_LIBS})
        endif()
    else()
        target_compile_definitions(air_cuda PRIVATE AIR_HAS_NVTX=0)
    endif()
    set_target_properties(air_cuda PROPERTIES
        CUDA_STANDARD 20
        CUDA_STANDARD_REQUIRED ON
        CUDA_EXTENSIONS OFF
        CUDA_SEPARABLE_COMPILATION ON
        POSITION_INDEPENDENT_CODE ON
    )
    air_enable_warnings(air_cuda)
    target_compile_options(air_cuda PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>)
else()
    add_library(air_cuda src/cuda/cuda_stub.cpp)
    target_link_libraries(air_cuda PUBLIC AIR::core)
    target_compile_definitions(air_cuda PUBLIC AIR_HAS_CUDA=0)
    air_enable_warnings(air_cuda)
endif()
add_library(AIR::cuda ALIAS air_cuda)
set_target_properties(air_cuda PROPERTIES EXPORT_NAME cuda)

target_include_directories(air_cuda
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_executable(air-cli src/cli/main.cpp)
target_link_libraries(air-cli PRIVATE AIR::core AIR::cuda)
air_enable_warnings(air-cli)

add_executable(air-server src/server/main.cpp src/server/protocol.cpp)
target_link_libraries(air-server PRIVATE AIR::core AIR::cuda)
air_enable_warnings(air-server)
target_compile_definitions(air-server PRIVATE AIR_WEB_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/web")

add_executable(air-bench src/benchmark/main.cpp)
target_link_libraries(air-bench PRIVATE AIR::core AIR::cuda)
air_enable_warnings(air-bench)

add_executable(air-qualify src/qualification/main.cpp)
target_link_libraries(air-qualify PRIVATE AIR::core AIR::cuda)
air_enable_warnings(air-qualify)

add_executable(air-verify src/verification/main.cpp)
target_link_libraries(air-verify PRIVATE AIR::core AIR::cuda)
air_enable_warnings(air-verify)

add_executable(air-strategy-probe src/strategy_probe/main.cpp)
target_link_libraries(air-strategy-probe PRIVATE AIR::core AIR::cuda)
air_enable_warnings(air-strategy-probe)

if(AIR_BUILD_TESTS)
    enable_testing()

    add_executable(air-core-tests tests/core_tests.cpp)
    target_link_libraries(air-core-tests PRIVATE AIR::core AIR::cuda)
air_enable_warnings(air-core-tests)

    add_executable(air-reference-tests tests/reference_tests.cpp)
    target_include_directories(air-reference-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(air-reference-tests PRIVATE AIR::core)
air_enable_warnings(air-reference-tests)

    add_executable(air-cuda-contract-tests tests/cuda_contract_tests.cpp)
    target_include_directories(air-cuda-contract-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(air-cuda-contract-tests PRIVATE AIR::core AIR::cuda)
air_enable_warnings(air-cuda-contract-tests)

    add_executable(air-serving-tests tests/serving_tests.cpp)
    target_link_libraries(air-serving-tests PRIVATE AIR::core AIR::cuda)
air_enable_warnings(air-serving-tests)

    add_executable(air-decision-contract-tests tests/decision_contract_tests.cpp)
    target_link_libraries(air-decision-contract-tests PRIVATE AIR::core)
air_enable_warnings(air-decision-contract-tests)

    add_executable(air-decision-serving-tests tests/decision_serving_tests.cpp)
    target_include_directories(air-decision-serving-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(air-decision-serving-tests PRIVATE AIR::core AIR::cuda)
    air_enable_warnings(air-decision-serving-tests)

    add_executable(air-sequence-state-store-tests tests/sequence_state_store_tests.cpp)
    target_include_directories(air-sequence-state-store-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(air-sequence-state-store-tests PRIVATE AIR::core)
    air_enable_warnings(air-sequence-state-store-tests)

    add_executable(air-protocol-tests tests/protocol_tests.cpp src/server/protocol.cpp)
    target_link_libraries(air-protocol-tests PRIVATE AIR::core AIR::cuda)
air_enable_warnings(air-protocol-tests)

    add_executable(air-scheduler-tests tests/scheduler_tests.cpp src/runtime/scheduler.cpp)
    target_include_directories(air-scheduler-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(air-scheduler-tests PRIVATE AIR::core)
    air_enable_warnings(air-scheduler-tests)

    add_executable(air-verification-tests tests/verification_tests.cpp)
    target_link_libraries(air-verification-tests PRIVATE AIR::core)
    air_enable_warnings(air-verification-tests)

    add_executable(air-hardware-topology-tests tests/hardware_topology_tests.cpp)
    target_link_libraries(air-hardware-topology-tests PRIVATE AIR::core AIR::cuda)
    air_enable_warnings(air-hardware-topology-tests)

    add_executable(air-observation-contract-tests tests/observation_contract_tests.cpp)
    target_include_directories(air-observation-contract-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(air-observation-contract-tests PRIVATE AIR::core)
    air_enable_warnings(air-observation-contract-tests)

    add_executable(air-strategy-lab-tests tests/strategy_lab_tests.cpp)
    target_link_libraries(air-strategy-lab-tests PRIVATE AIR::core)
    air_enable_warnings(air-strategy-lab-tests)

    add_executable(air-workload-contract-tests tests/workload_contract_tests.cpp)
    target_link_libraries(air-workload-contract-tests PRIVATE AIR::core)
    air_enable_warnings(air-workload-contract-tests)

    add_executable(air-resource-contract-tests tests/resource_contract_tests.cpp)
    target_link_libraries(air-resource-contract-tests PRIVATE AIR::core)
    air_enable_warnings(air-resource-contract-tests)

    add_executable(air-invocation-contract-tests tests/invocation_contract_tests.cpp)
    target_link_libraries(air-invocation-contract-tests PRIVATE AIR::core)
    air_enable_warnings(air-invocation-contract-tests)

    add_executable(air-flux2-semantic-contract-tests tests/flux2_semantic_contract_tests.cpp)
    target_link_libraries(air-flux2-semantic-contract-tests PRIVATE AIR::core)
    air_enable_warnings(air-flux2-semantic-contract-tests)

    add_executable(air-flux2-plan-contract-tests tests/flux2_plan_contract_tests.cpp)
    target_link_libraries(air-flux2-plan-contract-tests PRIVATE AIR::core)
    air_enable_warnings(air-flux2-plan-contract-tests)

    add_test(NAME air-core-tests COMMAND air-core-tests)
    add_test(NAME air-reference-tests COMMAND air-reference-tests)
    add_test(NAME air-cuda-contract-tests COMMAND air-cuda-contract-tests)
    add_test(NAME air-serving-tests COMMAND air-serving-tests)
    add_test(NAME air-decision-contract-tests COMMAND air-decision-contract-tests)
    add_test(NAME air-decision-serving-tests COMMAND air-decision-serving-tests)
    add_test(NAME air-sequence-state-store-tests COMMAND air-sequence-state-store-tests)
    add_test(NAME air-protocol-tests COMMAND air-protocol-tests)
    add_test(NAME air-scheduler-tests COMMAND air-scheduler-tests)
    add_test(NAME air-verification-tests COMMAND air-verification-tests)
    add_test(NAME air-hardware-topology-tests COMMAND air-hardware-topology-tests)
    add_test(NAME air-observation-contract-tests COMMAND air-observation-contract-tests)
    add_test(NAME air-strategy-lab-tests COMMAND air-strategy-lab-tests)
    add_test(NAME air-workload-contract-tests COMMAND air-workload-contract-tests)
    add_test(NAME air-resource-contract-tests COMMAND air-resource-contract-tests)
    add_test(NAME air-invocation-contract-tests COMMAND air-invocation-contract-tests)
    add_test(NAME air-flux2-semantic-contract-tests COMMAND air-flux2-semantic-contract-tests)
    add_test(NAME air-flux2-plan-contract-tests COMMAND air-flux2-plan-contract-tests)
endif()


if(AIR_BUILD_RESEARCH)
    message(STATUS
        "AIR_BUILD_RESEARCH compatibility mode enabled; "
        "research workflows use air-verify, air-bench, and air-strategy-probe")
endif()

include(GNUInstallDirs)
install(TARGETS air_core air_cuda air-cli air-server air-bench air-qualify air-verify air-strategy-probe
    EXPORT AIRTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/air DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY web/ DESTINATION ${CMAKE_INSTALL_DATADIR}/air/web)
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DATADIR}/air/docs)
install(FILES README.md ROADMAP.md LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/air)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AIRConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/AIRConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/AIRConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(EXPORT AIRTargets
    FILE AIRTargets.cmake
    NAMESPACE AIR::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AIR
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/AIRConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/AIRConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AIR
)
