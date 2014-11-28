/* Wrapper code for handling Lua configuration files */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <libwr/trace.h>

#define HAL_CONFIG_FILE "/wr/etc/wrsw_hal.conf"

static lua_State *cfg_file = NULL;
static char *extra_cmdline = NULL;

static char *hal_config_file = HAL_CONFIG_FILE;

/* Sets the path to the HAL config file */
void hal_config_set_config_file(const char *str)
{
	hal_config_file = strdup(str);
}

/* Appends extra Lua code to the contents of the configuration file to
   be parsed. Can be used to execute inline config given as a command
   line parameter. */
int hal_config_extra_cmdline(const char *str)
{
	extra_cmdline = strdup(str);
	return 0;
}

/* Parses the HAL configuration file */
int hal_parse_config()
{
	int ret;

	TRACE(TRACE_INFO, "Parsing wrsw_hal configuration file: %s", HAL_CONFIG_FILE);

	cfg_file = lua_open();

	luaL_openlibs(cfg_file);

	/* Just execute the config file as a regular Lua script. The
	   contents of the file will be ordinary Lua variables
	   accessible via lua_State. */
	ret = luaL_dofile(cfg_file, hal_config_file);

	/* Declare a Lua "helper" function for regexp searching global
	   variables - it's much easier to implement in Lua than in
	   plain C. */
	ret |= luaL_dostring(cfg_file, "\
	function get_var(name)	 \
	local t = _G					\
	for w in name:gmatch(\"([%w_]+)\\.?\") do	\
	t = t[w]					\
	end						\
	return t					\
	end");

	/* Execute extra code from the command line */
	if (extra_cmdline)
		ret |= luaL_dostring(cfg_file, extra_cmdline);

	if (ret) {
		TRACE(TRACE_ERROR, "Error parsing the configuration file: %s",
		      lua_tostring(cfg_file, -1));
		return -1;
	}

	return 0;
}

/* Looks up for a global variable (name). If it's found, it is pushed
   on the Lua stack and the function returns 0, otherwise a negative
   error code is returned. */
static int global_get_var(const char *name)
{
	lua_getglobal(cfg_file, "get_var");
	lua_pushstring(cfg_file, name);

	if (lua_pcall(cfg_file, 1, 1, 0) != 0)
		return -1;
	return 0;
}

/* Retreives an integer variable (name) and stores it at
   (value). Returns 0 on success, -1 if the variable was not found or
   has invalid format. */
int hal_config_get_int(const char *name, int *value)
{
	if (global_get_var(name) < 0)
		return -1;
	if (!lua_isnumber(cfg_file, -1))
		return -1;
	*value = (int)lua_tonumber(cfg_file, -1);
	return 0;
}

/* Same as above, but for double floating point numbers. */
int hal_config_get_double(const char *name, double *value)
{
	if (global_get_var(name) < 0)
		return -1;
	if (!lua_isnumber(cfg_file, -1))
		return -1;
	*value = (double)lua_tonumber(cfg_file, -1);
	return 0;
}

/* Same as above, but for null-terminated strings */
int hal_config_get_string(const char *name, char *value, int max_len)
{
	if (global_get_var(name) < 0)
		return -1;
	if (!lua_isstring(cfg_file, -1))
		return -1;
	strncpy(value, lua_tostring(cfg_file, -1), max_len);
	return 0;
}

/* Iterates a particular section (section) in the config file for its
 * subsections. For a file containing:

   ports = {
   wru0 = {
   ...
   };

   wr1 = {
   ...
   };

   wr2 = {
   ...
   };
   };

   calling hal_config_iterate("ports", 0, sub, strlen(sub)) will
   return 1 and sub == "wr0",

   calling hal_config_iterate("ports", 1, sub, strlen(sub)) will
   return 1 and sub == "wr1",

   hal_config_iterate("ports", 3, sub, strlen(sub)) will return 0, as
   there will be no more subsections in the "ports" section.
 */

int hal_config_iterate(const char *section, int index, char *subsection,
		       int max_len)
{
	int i = 0;

	if (global_get_var(section) < 0)
		return -1;

	lua_pushnil(cfg_file);	/* first key */
	while (lua_next(cfg_file, -2) != 0) {
		/* uses 'key' (at index -2) and 'value' (at index -1) */

		char *key_type = (char *)lua_typename(cfg_file,
						      lua_type(cfg_file, -1));
		if (!strcmp(key_type, "table") && i == index) {
			strncpy(subsection, lua_tostring(cfg_file, -2),
				max_len);
			return 1;
		} else if (!strcmp(key_type, "string") && i == index) {
			strncpy(subsection, lua_tostring(cfg_file, -1),
				max_len);
			return 1;
		}

		i++;

		/* removes 'value'; keeps 'key' for next iteration */
		lua_pop(cfg_file, 1);
	}

	return 0;
}
