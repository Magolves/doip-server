# Paths
set(VENV_DIR ${CMAKE_CURRENT_BINARY_DIR}/venv)
set(PYTHON_EXECUTABLE python3)  # or specify a full path

# Create venv
add_custom_command(
    OUTPUT ${VENV_DIR}
    COMMAND ${PYTHON_EXECUTABLE} -m venv ${VENV_DIR}
    COMMENT "Creating Python virtual environment"
)

# Install packages
add_custom_command(
    OUTPUT ${VENV_DIR}/installed
    COMMAND ${VENV_DIR}/bin/pip3 install udsoncan doipclient
    DEPENDS ${VENV_DIR}
    COMMENT "Installing Python packages in venv"
    BYPRODUCTS ${VENV_DIR}/installed
)

# Custom target to set up venv and packages
add_custom_target(
    setup_venv ALL
    DEPENDS ${VENV_DIR}/installed
)
