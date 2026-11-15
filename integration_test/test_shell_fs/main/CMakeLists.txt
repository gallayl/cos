set(SHELL_FS_FIXTURE_AUTOEXEC "${CMAKE_CURRENT_LIST_DIR}/../fixtures/autoexec.be")

file(READ "${SHELL_FS_FIXTURE_AUTOEXEC}" SHELL_FS_AUTOEXEC_HEX HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " SHELL_FS_AUTOEXEC_BYTES "${SHELL_FS_AUTOEXEC_HEX}")
file(SIZE "${SHELL_FS_FIXTURE_AUTOEXEC}" SHELL_FS_AUTOEXEC_SIZE)

set(SHELL_FS_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${SHELL_FS_GENERATED_DIR}")
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/generated_test_scripts.h.in"
    "${SHELL_FS_GENERATED_DIR}/generated_test_scripts.h"
    @ONLY
)

idf_component_register(
    SRCS "test_shell_fs.c"
    INCLUDE_DIRS "." "${SHELL_FS_GENERATED_DIR}"
    REQUIRES nvs_flash filesystem shell
)
