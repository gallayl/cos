#pragma once

/**
 * Reset the mock command registry (clears all registered commands).
 */
void mock_console_reset(void);

/**
 * Look up a registered command by name and invoke it.
 * Returns the command's return value, or -1 if not found.
 */
int mock_console_run_cmd(const char *name, int argc, char **argv);
