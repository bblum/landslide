/**
 * @file timetravel-simics.c
 * @brief simics implementation of time travel using bookmarks
 * @author Ben Blum
 */

#ifndef MODULE_NAME
#error "don't compile this file directly; compile timetravel.c"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> /* for open */
#include <unistd.h> /* for write */
#include <string.h> /* for memcmp, strlen */

/* The bookmark name is this prefix concatenated with the hexadecimal address
 * of the choice struct (no "0x"). */
#define BOOKMARK_PREFIX "landslide"
#define BOOKMARK_SUFFIX_LEN ((int)(2*sizeof(unsigned long long))) /* for hex */
#define BOOKMARK_MAX_LEN (strlen(BOOKMARK_PREFIX) + 16)

#define CMD_BOOKMARK "set-bookmark"
#define CMD_DELETE   "delete-bookmark"
#define CMD_SKIPTO   "skip-to"
#define CMD_BUF_LEN 64
#define MAX_CMD_LEN (MAX(strlen(CMD_BOOKMARK), \
			 MAX(strlen(CMD_DELETE),strlen(CMD_SKIPTO))))

/* Running commands is done by use of SIM_run_alone. We write the command out to
 * a file, and pause simics's execution. Our wrapper will cause the command file
 * to get executed. (This is necessary because simics refuses to run "skip-to"
 * from execution context.) */
struct cmd_packet {
	const char *file;
	const char *cmd;
	unsigned long long label;
};

static void run_command_cb(lang_void *addr)
{
	struct cmd_packet *p = (struct cmd_packet *)addr;
	char buf[CMD_BUF_LEN];
	int ret;
	int fd = open(p->file, O_CREAT | O_WRONLY | O_APPEND,
		      S_IRUSR | S_IWUSR);
	assert(fd != -1 && "failed open command file");

	/* Generate command */
	assert(CMD_BUF_LEN > strlen(p->cmd) + 1 + BOOKMARK_MAX_LEN);
	ret = scnprintf(buf, CMD_BUF_LEN, "%s " BOOKMARK_PREFIX "%.*llx\n",
		        p->cmd, BOOKMARK_SUFFIX_LEN, p->label);
	assert(ret > 0 && "failed scnprintf");
	ret = write(fd, buf, ret);
	assert(ret > 0 && "failed write");

	if (buf[strlen(buf)-1] == '\n') {
		buf[strlen(buf)-1] = '\0';
	}
	lsprintf(INFO, "Using file '%s' for cmd '%s'\n", p->file, buf);

	/* Clean-up */
	ret = close(fd);
	assert(ret == 0 && "failed close");

	MM_FREE(p);
}

static void run_command(const char *file, const char *cmd, struct timetravel_hax *th)
{
	struct cmd_packet *p = MM_XMALLOC(1, struct cmd_packet);

	p->cmd   = cmd;
	p->label = (unsigned long long)th;
	p->file  = file;

	SIM_break_simulation(NULL);
	SIM_run_alone(run_command_cb, (lang_void *)p);
}

void timetravel_set(struct timetravel_state *ts, struct timetravel_hax *th)
{
	run_command(ts->file, CMD_BOOKMARK, th);
}

void timetravel_jump(struct timetravel_state *ts, struct timetravel_hax *th,
		     unsigned int tid, bool txn, unsigned int xabort_code)
{
	run_command(ts->file, CMD_SKIPTO, th);
}

void timetravel_delete(struct timetravel_state *ts, struct timetravel_hax *th)
{
	run_command(ts->file, CMD_DELETE, th);
}
