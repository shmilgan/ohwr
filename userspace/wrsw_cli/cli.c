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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <readline.h>
#include <history.h>
#include <errno.h>

#include <rtu_fd_proxy.h>

#include "cli.h"
#include "cli_commands.h"
#include "cli_snmp.h"
#include "cli_commands_utils.h"


struct cli_word {
    char word[MAX_WORD_LEN];
};

/* Stream buffer to store the user input */
static struct cli_word stream[MAX_WORDS_IN_LINE];

/* Reference needed by the readline library function for command
   autocompletion */
static struct cli_shell *_cli;

/* Internal functions */
static void usage(char *name);
static int parse_stream(char *input);
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
 * \brief Turns the string introduced by the user into an array of words.
 * It alternatively detects white spaces and words. Each word is stored in one
 * of the elements of the stream buffer.
 * @param input string introduced by the user.
 * @return number of words detected.
 */
static int parse_stream(char *input)
{
    char *word = input;
    int num_words = 0;
    int i = 0;
    int word_len;
    char *word_start;

    /* Clear buffer */
    memset(stream, '\0', MAX_WORD_LEN * MAX_WORDS_IN_LINE);

    for (i = 0; i < MAX_WORDS_IN_LINE; i++) {
        /* Reset word length value when entering the loop */
        word_len = 0;

        while (*word && isspace(*word)) /* Detect white spaces */
            word++;

        if (!*word)
            break;

        word_start = word;

        while (*word && !isspace(*word)) { /* Detect characters */
            word++;
            word_len++;
        }

        if (word_len > MAX_WORD_LEN) /* Avoid buffer overflow */
            word_len = MAX_WORD_LEN - 1;

        /* Copy the detected command to the array */
        memcpy(stream[i].word, word_start, word_len);
        num_words = i + 1;

        /* Be sure that we are not yet at the end of the string */
        if (!*word)
            break;
    }

    return num_words;
}

/**
 * \brief Generator function for command completion. It generates the list of
 * possible matches. This function is called from rl_completion_matches,
 * returning a string each time, until no more matches are found.
 * @param cli CLI interpreter.
 * @param text the word to compete.
 * @param state lets us know whether to start from scratch; it's zero the first
 * time this function is called, so without any state (i.e. STATE == 0),
 * then we start at the top of the list.
 * @return commands that match the TEXT. "Returns NULL to inform
 * rl_completion_matches that there are no more possibilities left" (see the
 * GNU readline documentation)
*/
static char *command_generator(const char *text, int state)
{
    static int len, num_cmds, is_arg;
    static struct cli_cmd *ref_cmd = NULL;

    struct cli_shell *cli = _cli;
    struct cli_cmd *cmd = NULL;
    struct cli_cmd *c_found = NULL;
    int i;


    if (!state) {
        len = strlen(text);
        is_arg = 0;

        /* Parse the line gathered so far */
        num_cmds = parse_stream(rl_line_buffer);

        /* Check for allocation errors */
        if (num_cmds < 0)
            return NULL;

        /* If present, check first that the previous commands are correct
           parents */
        ref_cmd = cli->root_cmd; /* When num_cmds == 0,
                                    or when num_cmds == 1 and len != 0 */
        for (i = 0; i < ((len == 0) ? num_cmds : (num_cmds - 1)); i++) {
            if (!cmd) {
                /* Start the search from root_cmd */
                c_found = cli_find_command(cli, cli->root_cmd, stream[i].word);
            } else {
                /* Search among the children */
                c_found = cli_find_command(cli, cmd->child, stream[i].word);
            }

            if (!c_found) {
                if (!cmd) /* The first command in the line is wrong */
                    return NULL;

                /* Check if it may be an argument */
                if (cmd->opt != CMD_NO_ARG) {
                    is_arg = ~is_arg;
                    ref_cmd = cmd->child;
                    continue;
                }
                return NULL;
            }

            ref_cmd = c_found->child;
            is_arg = 0;
            cmd = c_found;
        }
    }

    cmd = ref_cmd;

    /* If we reach the end of the commands list, we exit */
    if (!cmd)
        return NULL;

    /* If it has to be an argument, no autocompletion is provided */
    if (cmd->parent)
        if ((is_arg == 0) && (cmd->parent->opt == CMD_ARG_MANDATORY))
            return NULL;

    /* Look for matches in the list of commands that share the parent */
    while (cmd) {
        if (strncmp(text, cmd->name, len) == 0) { /* Match found */
            ref_cmd = cmd->next; /* Save the reference cmd for the next loop */
            return (strdup(cmd->name));
        }
        cmd = cmd->next;
    }
    return NULL;
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

    matches = rl_completion_matches(text, command_generator);
    return matches;
}


/**
 * \brief Fatal error; CLI is unable to work properly. Exit CLI.
 * @param error Error code.
 */
void cli_fatal(int error)
{
    fprintf(stderr, "Fatal error %d: %s\n", error, strerror(error));
    exit(1);
}


/**
 * \brief Initialization function.
 * It allocates memory for the CLI interpreter structure and builds the commands
 * tree. It also creates a client for IPCs with the RTU daemon and initializes
 * the SNMP operational parameters.
 * @return pointer to the new created CLI interpreter structure. If NULL,
 * something is wrong and the program must be exited.
 */
struct cli_shell *cli_init(void)
{
    struct cli_shell *cli;
    struct minipc_ch *client;

    /* Set up the client for the RTU proxy */
    errno = 0;
    rtu_fdb_proxy_init("rtu_fdb");
    client = (struct minipc_ch *)rtu_fdb_proxy_connected();
    minipc_set_logfile(client, NULL); /* no log file */

    /* allocate memory for the CLI structure */
    cli = calloc(1, sizeof(struct cli_shell));
    if (!cli)
        cli_fatal(ENOMEM);

    /* Build the comands tree */
    cli_build_commands_tree(cli);

    /* Initialize the readline completer to our own function */
    rl_attempted_completion_function = completion;

    /* Do not attempt filename completion. We don't want that the default
       filename generator function be called when no matches are found. */
    rl_completion_entry_function = completion;

    /* Set the prompt */
    cli_build_prompt(cli);

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
    char buf[MAX_HOSTNAME_LEN + PROMPT_LEN + 1];

    if (!cli->hostname)
        cli->hostname = strdup(DEFAULT_HOSTNAME);

    if (cli->prompt)
        free(cli->prompt);

    strcpy(buf, cli->hostname);
    strcat(buf, EXEC_PROMPT);
    cli->prompt = strdup(buf);
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

    for (c = top_cmd; c; c = c->next)
        if (!strcmp(c->name, cmd))
            return c; /* Command found */

    return NULL;
}


/**
 * \brief Runs the command string typed by the user.
 * It first parses the command line to detect the words in the string. Then it
 * looks in the commands tree for commnads matching the words, and finally it
 * tries to execute the command. It also detects the help (or ?) command.
 * @param cli CLI interpreter.
 * @param input string typed by the user.
 */
void cli_run_command(struct cli_shell *cli, char *input)
{
    int num_cmds = 0;
    struct cli_cmd *cmd = NULL;
    struct cli_cmd *c_found = NULL;
    char *h = "help";
    char *q = "?";
    int i = 0;
    int valc = 0;
    char *valv[MAX_WORDS_IN_LINE] = {0};
    int arg = 0;  /* Flag to know whether we are looking for a command or
                     an argument */

    /* Parse input string and store words in the stream buffer */
    num_cmds = parse_stream(input);
    if (num_cmds == 0)
        return;

    /* Find the subcommands in the commands array, and evaluate them if
       possible */
    for (i = 0; i < num_cmds; i++) {
        if (!cmd) {
            /* Start the search from root_cmd */
            c_found = cli_find_command(cli, cli->root_cmd, stream[i].word);
        } else {
            /* Search among the children */
            c_found = cli_find_command(cli, cmd->child, stream[i].word);
        }

        /* If a matching subcommand has not been found, maybe the user has asked
           for help or maybe we have an argument for this command */
        if (!c_found) {
            if (!strcmp(stream[i].word, h) || !strcmp(stream[i].word, q)) {
                cli_cmd_help(cli, cmd);
                return;
            }
            if (!cmd || cmd->opt == CMD_NO_ARG) {
                printf("Error. Command %s does not exist.\n", stream[i].word);
                return;
            }
            if (arg) {
                /* It can be an argument. The handler will check
                   wether this is a valid argument or not */
                valc++;
                valv[(valc-1)] = stream[i].word;
                arg = 0; /* Next word can't be an arg, but a cmd */
                continue;
            }
            printf("Error. Command %s does not exist.\n", stream[i].word);
            return;
        }

        /* If a previous subcommand has been detected, check that it is a valid
           parent */
        if (cmd) {
            /* But first be sure that the found subcommand does have a parent */
            if (c_found->parent) {
                if (memcmp(cmd, c_found->parent, sizeof(struct cli_cmd))) {
                    printf("Error. Wrong command syntax.\n");
                    return;
                }
            } else {
                printf("Error. Wrong command syntax.\n");
                return;
            }
        }

        cmd = c_found;
        arg = 1; /* Next word can be an argument */
    }

    /* Run the handler associated to the last subcommand detected. Note that
       empty commands will be never processed, since the readline ignores the
       empty commands */
    if (cmd->handler) {
        cmd->handler(cli, valc, valv);
    } else {
        printf("Nothing to do for this command. Type '");
        for (i = 0; i < num_cmds; i++)
            printf("%s ", stream[i].word);
        printf("help' for usage.\n");
    }
}


/**
 * \brief Main loop.
 * Waits for the user to write a command, and then tries to run it.
 */
void cli_main_loop(struct cli_shell *cli)
{
    char *line = NULL;
    char *input = NULL;

    /* Init loop */
    while (1) {
        /* Use readline function to let Command Line Editing */
        line = readline(cli->prompt);
        if (!line)
            break;

        input = line;
        if (!*input) /* No need to process empty strings */
            continue;

        /* Save the string to the history */
        add_history(input);

        /* Evaluate the commands line inserted by the user and try to run
           the command */
        cli_run_command(cli, input);

        /* Clear the buffer for the next iteration */
        free(line);
    }
}


/**
 * \brief Main function.
 */
int main(int argc, char **argv)
{
    int op;
    char *optstring;
    char *version = VERSION;
    char *username = NULL;
    char *password = NULL;
    struct cli_shell *cli;


    /* Capture options */
    if (argc > 1) {
        /* Parse options */
        optstring = "hp:u:";
        while ((op = getopt(argc, argv, optstring)) != -1) {
            switch(op) {
            case 'h':
                usage(argv[0]);
                break;
            case 'p':
                password = optarg;
                break;
            case 'u':
                username = optarg;
                break;
            default:
                usage(argv[0]);
                break;
            }
        }
    }

    if (!username || !password)
        usage(argv[0]);

    /* Init SNMP */
    if (cli_snmp_init(username, password) < 0)
        cli_fatal(ENOMEM);

    /* Initialize the CLI */
    _cli = cli = cli_init();

    /* Print welcome message */
    printf("\twrsw_cli version %s. Copyright (C) 2011, CERN\n", version);

    /* Init main loop */
    cli_main_loop(cli);

	return 0;
}
