/* 
 *  Implenment simple command line interface
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <cstdbool>
#include <sys/select.h>
#include <string>

#define RIO_BUFSIZE 8192
#define MAX_QUIT_FUNC 10

/* Simulation flag of console option */
bool simulation = false;

/* Function definition  of each command */
typedef bool (*cmd_function)(int argc, std::string *argv);

/*  
 *  Info about each command.
 *  Organized as linked list in alphabetical order.
 */
typedef struct CELE cmd_ele, *cmd_ptr;
struct CELE {
    std::string name;
    cmd_function operation;
    std::string documentation;
    cmd_ptr next;
};

/* 
 *  (Optional) 
 *  when parameter changes, 
 *  this function will be provoked
 */
typedef void (*setter_function)(int oldval);

/* Info about each INTEGER-valued parameter */
typedef struct PELE para_ele, *para_ptr;
struct PELE {
    std::string name;
    int *valp;
    std::string documentation;
    /* Function that will be provoked when parameter changes */
    setter_function setter;
    para_ptr next;
};

/* 
 *  Implenment buffered I/O ugin variant of RIO package from CS:APP
 *  Must create stack of buffers to handle I/O with nested source commands
 */
typedef struct RIO_ELE rio_t, *rio_ptr;
struct RIO_ELE {
    int fd;                 /* File descriptor */
    int cnt;                /* Unread bytes in internal buffer */
    char *bufptr;           /* Next unread byte in internal buffer */
    char buf[RIO_BUFSIZE];  /* Internal buffer */
    rio_ptr prev;           /* Next element in stack */
};

class console
{
    private:
        /* Class members */
        cmd_ptr cmd_list;
        para_ptr para_list;       

        /* Time */
        double first_time;
        double last_time;

        /* RIO */
        rio_ptr buf_stack;
        char linebuf[RIO_BUFSIZE];

        /* Quit functions */
        cmd_function quit_helpers[MAX_QUIT_FUNC];
        int quit_helper_cnt;

        bool push_file(std::string fname);
        void pop_file();
        bool interpret_cmda(int argc, std::string *argv);
        bool interpret_cmd(std::string cmdline);

        void record_error();

    public:
        bool block_flag;
        bool prompt_flag;
        /* Whether timing a command that has console blocked */
        bool block_timing;
        /* Maximum file descriptor */
        int fd_max;
        int err_limit;
        int err_cnt;
        bool echo;
        bool quit_flag;
        std::string prompt;

        /* Functions */
        console();
        ~console();

        /* Initialization of console */
        void init();

        /* Add command to the command linked list */
        void add_cmd(std::string name, 
                     cmd_function operation, 
                     std::string documentation);
        
        /* Add parameter to the parameter linked list */
        void add_para(std::string name, 
                      int *valp,
                      setter_function setter,
                      std::string documentation);
        
        /* Retrieve integer from text and store at loc */
        bool retrieveInt(std::string vname, int *loc);

        /* Add function to be part of program exit */
        void add_quit_helper(cmd_function quit_f);

        /* Turn echo on/off*/
        void set_echo(bool swi);

        /* Return true if no errors occured */
        bool finish_cmd();

        /* Command processing in program using select */
        int cmd_select(int nfds,
                       fd_set *readfds,
                       fd_set *writefds,
                       fd_set *exceptfds,
                       struct timeval *timeout);

        /* Run command loop */
        bool run_console(std::string infile_name);
};


#endif