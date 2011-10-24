/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: Basic CLI implementation.
 *              Initialization of data structures.
 *              Main loop.
 *              Command parsing and command line editing capabilities.
 *
 *
 *              Part of the code written here is based on ideas taken from the
 *              LGPL libcli package (https://github.com/dparrish/libcli.git)
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <readline.h>
#include <history.h>

#include "cli.h"
#include "cli_commands.h"
#include "cli_snmp.h"



/* Reference needed by the readline library function for command
   autocompletion */
static struct cli_shell *_cli;

/* Internal functions */
static void usage(char *name);
static int parse_string(char *string, char *commands[]);
static void free_cmds_string(int num_cmds, char *commands[]);
static char **completion(char *text, int start, int end);


static void usage(char *name)
{
    fprintf(stderr,
        "Usage: %s [-h] -p <password> -u <username>\n"
        "\t-h   help\n"
        "\t-p   The password for the SNMPv3 authentication\n"
        "\t-u   The username for the SNMPv3 authentication\n",
        name);
    exit(1);
}


/**
 * \brief Turns the string introduced by the user into an array of commands.
 * It alternatively detects white spaces and words. Each word is stored in one
 * of the elements of the commands array.
 * @param string string introduced by the user.
 * @param commands array to store the detected commands.
 * @return number of commands detected.
 */
static int parse_string(char *string, char *commands[])
{
    char *word = string;
    int num_cmds = 0;
    int i = 0;
    int cmd_length = 0;
    char *cmd_start;

    for (i = 0; i < MAX_CMDS_IN_LINE; i++) {
        while (*word && isspace(*word)) { /* Detect white spaces */
            word++;
        }

        if (!*word)
            break;

        cmd_start = word;

        while (*word && !isspace(*word)) { /* Detect words */
            word++;
            cmd_length++;
        }

        /* Allocate memory for the command */
        commands[i] = (char*)malloc(cmd_length + 1);
        if (!commands[i]) {
            return -1;
        }

        /* Copy the detected command to the array */
        memcpy(commands[i], cmd_start, cmd_length);
        commands[i][cmd_length] = 0;
        num_cmds = i + 1;

        /* Clean for the next loop */
        cmd_length = 0;

        /* Be sure that we are not yet at the end of the string */
        if (!*word)
            break;
    }

    return num_cmds;
}

/**
 * \brief It frees the temporal commands array.
 * @param num_cmds The number of elements of the array.
 * @param commands array where the detected commands are stored.
 */
static void free_cmds_string(int num_cmds, char *commands[])
{
    int i = 0;
    for (i = 0; i < num_cmds; i++)
        free(commands[i]);
}


/**
 * \brief Generator function for command completion.
 * @param cli CLI interpreter.
 * @param text the word to compete.
 * @param state lets us know whether to start from scratch; it's zero the first
 * time this function is called, so without any state (i.e. STATE == 0),
 * then we start at the top of the list.
*/
static char *command_generator(const char *text, int state)
{
    static int len, num_cmds, is_arg;
    static struct cli_cmd *ref_cmd = NULL;

    struct cli_shell *cli = _cli;
    char *commands[MAX_CMDS_IN_LINE] = {0};
    struct cli_cmd *cmd = NULL;
    struct cli_cmd *c_found = NULL;
    int i;


    if (!state) {
        len = strlen(text);
        is_arg = 0;

        /* Parse the line gathered so far */
        num_cmds = parse_string(rl_line_buffer, commands);

        /* Check for allocation errors */
        if (num_cmds < 0)
            return ((char*)NULL);

        /* If present, check first that the previous commands are correct
           parents */
        if (num_cmds >= 0) {
            ref_cmd = cli->root_cmd; /* When num_cmds == 0,
                                        or when num_cmds == 1 and len != 0 */
            for (i = 0; i < ((len == 0) ? num_cmds : (num_cmds - 1)); i++) {
                if (!cmd) {
                    /* Start the search from root_cmd */
                    c_found = cli_find_command(cli, cli->root_cmd, commands[i]);
                } else {
                    /* Search among the children */
                    c_found = cli_find_command(cli, cmd->child, commands[i]);
                }

                if (!c_found) {
                    if (!cmd) { /* The first command in the line is wrong */
                        free_cmds_string(num_cmds, commands);
                        return ((char*)NULL);
                    }

                    /* Check if it may be an argument */
                    if (cmd->opt != CMD_NO_ARG) {
                        is_arg = ~is_arg;
                        ref_cmd = cmd->child;
                        continue;
                    } else {
                        free_cmds_string(num_cmds, commands);
                        return ((char*)NULL);
                    }
                } else {
                    ref_cmd = c_found->child;
                }
                is_arg = 0;
                cmd = c_found;
            }
        }
    }

    cmd = ref_cmd;

    /* If we reach the end of the commands list, we exit */
    if (!cmd) {
        free_cmds_string(num_cmds, commands);
        return ((char*)NULL);
    }

    /* If it has to be an argument, no autocompletion is provided */
    if (cmd->parent) {
        if ((is_arg == 0) && (cmd->parent->opt == CMD_ARG_MANDATORY)) {
            free_cmds_string(num_cmds, commands);
            return ((char*)NULL);
        }
    }

    /* Look for matches in the list of commands that share the parent */
    while (cmd) {
        if (strncmp(text, cmd->name, len) == 0) { /* Match found */
            ref_cmd = cmd->next; /* Save the reference cmd for the next loop */
            return (strdup(cmd->name));
        }
        cmd = cmd->next;
    }

    free_cmds_string(num_cmds, commands);
    return ((char*)NULL);
}


/**
 * \brief Attempt to complete on the contents of TEXT. It calls the
 * command_generator function until it returns a NULL pointer.
 * @param text the word to complete.
 * @param start
 * @param end START and END bound the region of rl_line_buffer that contains
 * the word to complete. We can use the entire contents of rl_line_buffer
 * in case we want to do some simple parsing. Return the array of matches,
 * or NULL if there aren't any.
*/
static char **completion(char *text, int start, int end)
{
    char **matches;

    matches = (char **)NULL;

    matches = rl_completion_matches(text, command_generator);

    return matches;
}


/**
 * \brief Initialization function.
 * It allocates memory for the CLI interpreter structure and builds the commands
 * tree.
 * @return pointer to the new created CLI interpreter structure. If NULL,
 * something is wrong and the program must be exited.
 */
struct cli_shell *cli_init(void)
{
    struct cli_shell *cli;

    cli = (struct cli_shell *)malloc(sizeof(struct cli_shell));
    if (!cli)
        cli_error(NULL, CLI_ALLOC_ERROR);

    /* Clear structure */
    memset(cli, 0, sizeof(struct cli_shell));

    /* Build the comands tree */
    cli_build_commands_tree(cli);

    /* Initialize the readline completer to our own function */
    rl_attempted_completion_function = (rl_completion_func_t *)completion;

    /* Do not attempt filename completion. We don't want that the default
       filename generator function be called when no matches are found. */
    rl_completion_entry_function = (rl_compentry_func_t *) completion;

    return cli;
}


/**
 * \brief Builds the prompt of the CLI.
 * The prompt consists of the configurable hostname of the switch followed by a
 * string representing the prompt itself.
 * @param cli CLI interpreter.
 */
void cli_build_prompt(struct cli_shell *cli)
{
    char buf[64];

    if (!cli->hostname)
        cli->hostname = strdup(DEFAULT_HOSTNAME);

    if (cli->prompt)
        free(cli->prompt);

    strcpy(&buf[0], cli->hostname);
    strcat(&buf[0], EXEC_PROMPT);
    cli->prompt = strdup(&buf[0]);
}


/**
 * \brief Find commands in the commads tree.
 * @param cli CLI interpreter.
 * @param top_cmd the command from which the search begins.
 * @param cmd command name to search.
 * @return pointer to the command found. NULL if the command is not found.
 */
struct cli_cmd *cli_find_command(
    struct cli_shell *cli, struct cli_cmd *top_cmd, char *cmd)
{
    struct cli_cmd *c;

    for (c = top_cmd; c; c = c->next) {
        if (!strcmp(c->name, cmd))
            return c; /* Command found */
    }

    return NULL;
}


/**
 * \brief Runs the command string typed by the user.
 * It first parses the command line to detect the words in the string. Then it
 * looks in the commands tree for commnads matching the words, and finally it
 * tries to execute the command. It also detects the help (or ?) command.
 * @param cli CLI interpreter.
 * @param string string typed by the user.
 */
void cli_run_command(struct cli_shell *cli, char *string)
{
    char *commands[MAX_CMDS_IN_LINE] = {0};
    int num_cmds = 0;
    struct cli_cmd *cmd = NULL;
    struct cli_cmd *c_found = NULL;
    char *h = "help";
    char *q = "?";
    int i = 0;
    int argc = 0;
    char *argv[] = {0};
    int arg = 0;  /* Flag to know whether we are looking for a command or
                     an argument */


    if (!*string)
        return;

    /* Parse command. The commands will be stored in the 'commands' array */
    num_cmds = parse_string(string, commands);

    /* Check for allocation erors */
    if (num_cmds < 0) {
        printf("Out of memory while proccessing the command.\n");
        return;
    }

    /* Find the commands in the commands array, and evaluate them if possible */
    for (i = 0; i < num_cmds; i++) {
        if (!cmd) {
            /* Start the search from root_cmd */
            c_found = cli_find_command(cli, cli->root_cmd, commands[i]);
        } else {
            /* Search among the children */
            c_found = cli_find_command(cli, cmd->child, commands[i]);
        }

        /* If a matching command has not been found, maybe the user has asked
           for help or we have an argument for this command */
        if (!c_found) {
            if (!strcmp(commands[i], h) || !strcmp(commands[i], q)) {
                cli_cmd_help(cli, cmd);
                free_cmds_string(num_cmds, commands);
                return;
            } else {
                if (!cmd) {
                    printf("Error. Command %s does not exist.\n", commands[i]);
                    free_cmds_string(num_cmds, commands);
                    return;
                } else {
                    if (cmd->opt == CMD_NO_ARG) {
                        printf("Error. Command %s does not exist.\n",
                                commands[i]);
                        free_cmds_string(num_cmds, commands);
                        return;
                    } else {
                        if (arg) {
                            /* It can be an argument. The handler will check
                               wether this is a valid argument or not */
                            argc++;
                            argv[(argc-1)] = commands[i];
                            arg = 0; /* Next word can't be an arg, but a cmd */
                            continue;
                        } else {
                            printf("Error. Command %s does not exist.\n",
                                    commands[i]);
                            free_cmds_string(num_cmds, commands);
                            return;
                        }
                    }
                }
            }
        }

        /* If a previous command has been detected, check that it is the parent
           command */
        if (cmd) {
            /* But first be sure that the command found does have a parent */
            if (c_found->parent) {
                if (memcmp(cmd, c_found->parent, sizeof(struct cli_cmd))) {
                    printf("Error. Wrong command syntax.\n");
                    free_cmds_string(num_cmds, commands);
                    return;
                }
            } else {
                printf("Error. Wrong command syntax.\n");
                free_cmds_string(num_cmds, commands);
                return;
            }
        }

        cmd = c_found;
        arg = 1; /* Next word can be an argument */
    }

    /* Run the handler associated to the last command detected */
    if (cmd) {
        if (cmd->handler) {
            cmd->handler(cli, argc, argv);
        } else {
            printf("Nothing to do for this command. Type '");
            for (i = 0; i < num_cmds; i++)
                printf("%s ", commands[i]);
            printf("help' for usage.\n");
        }
    }

    free_cmds_string(num_cmds, commands);
}


/**
 * \brief Error handling.
 * It handles the fatal errors, i.e. those errors that force the program to
 * exit. It uses the cli_cmd_exit function to de-allocate data structures.
 * @param cli CLI interpreter.
 * @param error_code Error code.
 */
void cli_error(struct cli_shell *cli, int error_code)
{
    switch (error_code) {
    case CLI_ALLOC_ERROR:
        fprintf(stderr, "Memory allocation error\n");
        break;
    case CLI_SNMP_INIT_ERROR:
        fprintf(stderr, "SNMP initialization error\n");
        break;
    case CLI_REG_ERROR:
        fprintf(stderr, "An error has occurred while registering the"
                " commands tree\n");
        break;
    case CLI_ERROR:
    default:
        fprintf(stderr, "Fatal error: %i\n", error_code);
        break;
    }

    /* Free memory */
    cli_cmd_exit(cli, 0, NULL);
}


/**
 * \brief Main loop.
 * Waits for the user to write a command, and then tries to run it.
 */
void cli_main_loop(struct cli_shell *cli)
{
    char *line = NULL;
    char *string = NULL;

    /* Init loop */
    while (1) {
        /* Clear the buffer. Remember that the readline function makes the
           allocation of the string internally */
        if (line) {
            free(line);
            line = NULL;
        }

        /* Use readline function to let Command Line Editing */
        line = readline(cli->prompt);
        if (!line)
            break;

        string = line;
        while (*string && isspace(*string)) /* Remove initial white spaces */
            string++;
        if (!*string)
            continue;

        /* Save the string to the history */
        add_history(string);

        /* Evaluate the commands line inserted by the user and try to run
           the command */
        cli_run_command(cli, string);
    }

    free(string);
}


/**
 * \brief Main function.
 */
int main(int argc, char **argv)
{
    int op;
    char *s, *name, *optstring;
    char *version = VERSION;
    char *username = NULL;
    char *password = NULL;
    struct cli_shell *cli;


    /* Strip out path from argv[0] if exists, and extract command name */
    for (name = s = argv[0]; s[0]; s++) {
        if (s[0] == '/' && s[1]) {
            name = &s[1];
        }
    }

    /* Capture options */
    if (argc > 0) {
        /* Parse options */
        optstring = "hp:u:";
        while ((op = getopt(argc, argv, optstring)) != -1) {
            switch(op) {
            case 'h':
                usage(name);
                break;
            case 'p':
                password = optarg;
                break;
            case 'u':
                username = optarg;
                break;
            default:
                usage(name);
                break;
            }
        }
    }

    if (!username && !password)
        usage(name);

    /* Initialize the CLI data */
    cli = cli_init();

    /* Init SNMP */
    if (cli_snmp_init(username, password) < 0)
        cli_error(cli,CLI_SNMP_INIT_ERROR);

    /* Reference for the readline library */
    _cli = cli;

    /* Set the prompt */
    cli_build_prompt(cli);

    /* Print welcome message */
    printf("\n\t****************************\n");
    printf("\t  wrsw_cli version %s      \n", version);
    printf("\t  Copyright (C) 2011, CERN  \n");
    printf("\t****************************\n\n");

    /* Init main loop */
    cli_main_loop(cli);

    /* Free memory when exiting the main loop */
    if (cli)
        cli_error(cli, CLI_ERROR);

	return 0;
}
