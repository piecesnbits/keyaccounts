# All cmake projects need these
cmake_minimum_required(VERSION 3.16)
project(keyaccounts)

# clsdk requires C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Libraries for building contracts and tests
find_package(clsdk REQUIRED)

# include_directories(keyaccounts ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Build keyaccounts.wasm contract
add_executable(keyaccounts keyaccounts.cpp)
target_link_libraries(keyaccounts eosio-contract-simple-malloc)

# Generate keyaccounts.abi
# This is a 2-step process:
#   * Build keyaccounts.abi.wasm. This must link to eosio-contract-abigen.
#   * Run the wasm to generate the abi
add_executable(keyaccounts-abigen keyaccounts.cpp)
target_link_libraries(keyaccounts-abigen eosio-contract-abigen)
add_custom_command(TARGET keyaccounts-abigen POST_BUILD
    COMMAND cltester keyaccounts-abigen.wasm >keyaccounts.abi
)

# These symlinks help vscode
execute_process(COMMAND ln -sf ${clsdk_DIR} ${CMAKE_CURRENT_BINARY_DIR}/clsdk)
execute_process(COMMAND ln -sf ${WASI_SDK_PREFIX} ${CMAKE_CURRENT_BINARY_DIR}/wasi-sdk)

# Generate compile_commands.json to aid vscode and other editors
set(CMAKE_EXPORT_COMPILE_COMMANDS on)