#include <iostream>
#include <sys/time.h>

#include "console.h"

/* static function */
static void init_time(double *time);
static double delta_time(double *time);

/* The commands that will be added */
static bool do_quit_cmd(int argc, std::string *argv);
static bool do_help_cmd(int argc, std::string *argv);
static bool do_option_cmd(int argc, std::string *argv);
static bool do_source_cmd(int argc, std::string *argv);
static bool do_log_cmd(int argc, std::string *argv);
static bool do_time_cmd(int argc, std::string *argv);
static bool do_comment_cmd(int argc, std::string *argv);

console::console(/* args */)
{
    // simulation = false;
    cmd_list = NULL;
    para_list = NULL;
    block_flag = false;
    prompt_flag = true;
    block_timing = false;
    fd_max = 0;
    err_limit = 5;
    err_cnt = 0;
    echo = false;
    quit_flag = false;
    prompt = "cmd> ";
    quit_helper_cnt = 0;

    cmd_list = NULL;
    para_list = NULL;
}

console::~console()
{
}

void console::init()
{
    /* Add command */
    add_cmd("help", do_help_cmd, 
            "                | Show documentation");
    add_cmd("option", do_option_cmd,
            " [name val]     | Display or set options");
    add_cmd("quit", do_quit_cmd,
            "                | Exit program");
    add_cmd("source", do_source_cmd,
            " file           | Read commands from source file");
    add_cmd("log", do_log_cmd, 
            " file           | Copy output to file");
    add_cmd("time", do_time_cmd, 
            " cmd arg ...    | Time command execution");
    add_cmd("#", do_comment_cmd, 
            " ...            | Display comment");

    /* Add parameter*/
    add_para("simulation", (int *) &simulation, NULL,
             "Start/Stop simulation mode");
    add_para("error", &err_limit, NULL,
             "Number of errors until exit");
    add_para("echo", (int *)&echo, NULL,
             "Do/Don't echo commands");

    buf_stack = NULL;
    init_time(&last_time);
    first_time = last_time;
}

void console::add_cmd(std::string name, 
                      cmd_function operation, 
                      std::string documentation)
{
    cmd_ptr next_cmd = cmd_list;
    cmd_ptr *last_loc = &cmd_list;
    while (next_cmd && name.compare(next_cmd->name) > 0)
    {
        last_loc = &next_cmd->next;
        next_cmd = next_cmd->next;
    }

    cmd_ptr ele = (cmd_ptr) malloc(sizeof(cmd_ele));
    if (!ele) {
        std::cerr << "Cannot malloc for new command" << std::endl;
        return;
    }
    ele->name = name;
    ele->operation = operation;
    ele->documentation = documentation;
    ele->next = next_cmd;
    *last_loc = ele;
}

void console::add_para(std::string name, 
              int *valp,
              setter_function setter,
              std::string documentation)
{
    para_ptr next_para = para_list;
    para_ptr *last_loc = &para_list;
    while (next_para && name.compare(next_para->name) > 0)
    {
        last_loc = &next_para->next;
        next_para = next_para->next;
    }

    para_ptr ele = (para_ptr) malloc(sizeof(para_ele));
    if (!ele) {
        std::cerr << "Cannot malloc for new parameter" << std::endl;
        return;
    }
    ele->name = name;
    ele->valp = valp;
    ele->documentation = documentation;
    ele->setter = setter;
    ele->next = next_para;
    *last_loc = ele;
}

static void init_time(double *time)
{
    (void) delta_time(time);
}

static double delta_time(double *time)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double current_time = tv.tv_sec + 1.0E-6 * tv.tv_usec;
    double delta = current_time - *time;
    *time = current_time;
    return delta;
}

/* Parse string from command line */
static std::string *parse_args(std::string line, int *argcp)
{
    /*
     * Must know the number of arguments.
     * Then, replace the white space with null characters
     */
    size_t len = line.length();
    char *buf = (char *)malloc((len + 1) * sizeof(char));
    // std::string src = line;
    char *dst = buf;
    bool skip = true;

    int c;
    int argc = 0;
    for (int i=0; i < line.length(); i++)
    {
        c=line[i];
        if (c=='\0')
            break;
        if (isspace(c)) {
            /* reach the end of a word */
            if (!skip) {
                *dst++ = '\0';
                skip = true;
            }
        } 
        else {
            /* start a new word */
            if (skip) {
                argc++;
                skip = false;
            }
            *dst++ = c;
        }
    }

    std::string *argv = new std::string[argc];
    if (!argv) {
        std::cerr << "new string array fail in console.cpp" << std::endl;
        return NULL;
    }
    char *src = buf;
    for (int i=0; i < argc; i++) {
        argv[i] = src;
        src += argv[i].length() + 1;
    }
    free(buf);
    *argcp = argc;
    return argv;
}

void console::record_error()
{
    err_cnt++;
    if (err_cnt >= err_limit) {
        std::cerr << "Error limit exceeded. Stopping command execution" << std::endl;
        quit_flag = true;
    }
}

/* Execute a command that has already been split into arguments */
bool console::interpret_cmda(int argc, std::string *argv)
{
    if (argc==0)
        return true;

    /* Try to find matching command */
    cmd_ptr next_cmd = cmd_list;
    bool ok = true;
    while (next_cmd && argv[0].compare(next_cmd->name) != 0)
        next_cmd = next_cmd->next;
    if (next_cmd) {// find matched command
        ok = next_cmd->operation(argc, argv);
        if (!ok)
            record_error();
    }
    else {
        std::cerr << "Unknown command " << argv[0] << std::endl;
        record_error();
        ok = false;
    }
    return ok;
}

/* Execute a command from a command line */
bool console::interpret_cmd(std::string cmdline)
{
    if (quit_flag)
        return false;
    
    int argc;
    std::string *argv = parse_args(cmdline, &argc);
    bool ok = interpret_cmda(argc, argv);
    return ok;
}

/* Set function to be executed as part of program exit */
void console::add_quit_helper(cmd_function qf)
{
    if (quit_helper_cnt < MAX_QUIT_FUNC)
        quit_helpers[quit_helper_cnt++] = qf;
    else
        std::cerr << "Exceed limit on quit helpers" << std::endl;
}

/* Set echo on/off */
void console::set_echo(bool on)
{
    echo = on ? 1 : 0;
}

/* Built-in commands */
