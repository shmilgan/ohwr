#ifndef __LIBWR_CONFIG_H__
#define __LIBWR_CONFIG_H__

/* Read a dot-config file, return 0 or -line, the unparsable line in file */
int libwr_cfg_read_file(char *dotconfig);

/* Diagnostic tool */
int libwr_cfg_dump(FILE *output);

/* Read a dot-config, verifying with Kconfig to complain for wrong names */
int libwr_cfg_read_verify_file(char *dotconfig, char *kconfig);

/* cfg_get() works for CONFIG_NTP_SERVER. Returns NULL or static storage. */
char *libwr_cfg_get(char *cfgname);

/* cfg_get2(), as above but with suboptions like "name=AXGE-3454-0531". */
char *libwr_cfg_get2(char *cfgname, char *subname);

/* We only convert the following types */
enum libwr_convert {
	LIBWR_INT,
	LIBWR_STRING,
	LIBWR_DOUBLE,
};

/*
 * The following is the most flexible one: subname can be NULL,
 * and the cfgname is a printf-format string (e.g. "PORT%02i_PARAMS");
 * finally, the value is sscanf'd in the library, with error check.
 * Returns 0 or -1 with errno set to EINVAL.
 */
int libwr_cfg_convert2(char *cfgname, char *subname,
			   enum libwr_convert, void *ret, ...)
	__attribute__((format(printf, 1, 5)));

#endif /* __LIBWR_CONFIG_H__ */
