set(SHELL_FS_FIXTURES_DIR "${CMAKE_CURRENT_LIST_DIR}/../fixtures")
set(SHELL_FS_CANONICAL_PING "${CMAKE_CURRENT_LIST_DIR}/../../../storage/flash_data/scripts/ping.be")

function(shell_fs_embed_file file_path var_prefix)
    file(READ "${file_path}" _hex HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " _bytes "${_hex}")
    file(SIZE "${file_path}" _size)
    set(${var_prefix}_BYTES "${_bytes}" PARENT_SCOPE)
    set(${var_prefix}_SIZE "${_size}" PARENT_SCOPE)
endfunction()

function(shell_fs_embed_fixture script_name var_prefix)
    shell_fs_embed_file("${SHELL_FS_FIXTURES_DIR}/${script_name}" ${var_prefix})
    set(${var_prefix}_BYTES "${${var_prefix}_BYTES}" PARENT_SCOPE)
    set(${var_prefix}_SIZE "${${var_prefix}_SIZE}" PARENT_SCOPE)
endfunction()

shell_fs_embed_fixture("autoexec.be" SHELL_FS_AUTOEXEC)
shell_fs_embed_fixture("berry_api.be" SHELL_FS_BERRY_API)
shell_fs_embed_fixture("berry_net.be" SHELL_FS_BERRY_NET)
shell_fs_embed_fixture("berry_input.be" SHELL_FS_BERRY_INPUT)
shell_fs_embed_file("${SHELL_FS_CANONICAL_PING}" SHELL_FS_PING)

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
