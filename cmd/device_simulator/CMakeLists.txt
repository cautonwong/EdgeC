# Device Simulator Product Build Script
# ------------------------------------------------------------------------------

set(PROD_NAME device_simulator)

# 1. Generate the contract library from the product's YAML file.
edge_add_contract(
    contract_${PROD_NAME}
    "${CMAKE_SOURCE_DIR}/contracts/products/device_simulator.yaml"
    "" # No product-specific extension file is needed for the simulator
)

# 2. Define the executable.
add_executable(${PROD_NAME} main.c)

# Add explicit dependency to ensure contracts.h is generated first
add_dependencies(${PROD_NAME} contract_${PROD_NAME})

# 3. Standardize the output path for the binary.
set_target_properties(${PROD_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/bin"
)

# 4. Link only against the contract library and essential lower-level libs.
target_link_libraries(${PROD_NAME} PRIVATE 
    contract_${PROD_NAME}
    edge_core # For logging and net_buf
    uv        # For the event loop
    pthread dl rt m
)

# 5. Configure installation rules for the product.
install_edge_product(${PROD_NAME})