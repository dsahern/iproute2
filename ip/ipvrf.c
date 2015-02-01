#define _ATFILE_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include "utils.h"
#include "ip_common.h"

static int usage(void)
{
	fprintf(stderr, "Usage: ip vrf exec ID cmd ...\n");
	exit(-1);
}

static int vrf_exec(int argc, char **argv)
{
	const char *cmd, *id;
	char vrf_path[MAXPATHLEN];
	int fd;

	if (argc < 1) {
		fprintf(stderr, "No vrf id specified\n");
		return -1;
	}
	if (argc < 2) {
		fprintf(stderr, "No command specified\n");
		return -1;
	}

	id = argv[0];
	cmd = argv[1];
	snprintf(vrf_path, sizeof(vrf_path), "/proc/%d/vrf", getpid());
	fd = open(vrf_path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Cannot open vrf file: %s\n",
			strerror(errno));
		return -1;
	}
	if (write(fd, id, strlen(id)) < 0) {
		fprintf(stderr, "Failed to set vrf id: %s\n",
			strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);

	fflush(stdout);

	if (batch_mode) {
		int status;
		pid_t pid;

		pid = fork();
		if (pid < 0) {
			perror("fork");
			exit(1);
		}

		if (pid != 0) {
			/* Parent  */
			if (waitpid(pid, &status, 0) < 0) {
				perror("waitpid");
				exit(1);
			}

			if (WIFEXITED(status)) {
				/* ip must return the status of the child,
				 * but do_cmd() will add a minus to this,
				 * so let's add another one here to cancel it.
				 */
				return -WEXITSTATUS(status);
			}

			exit(1);
		}
	}

	if (execvp(cmd, argv + 1)  < 0)
		fprintf(stderr, "exec of \"%s\" failed: %s\n",
			cmd, strerror(errno));
	_exit(1);
}

int do_vrf(int argc, char **argv)
{
	if (*argv == NULL)
		return usage();

	if (matches(*argv, "help") == 0)
		return usage();

	if (matches(*argv, "exec") == 0)
		return vrf_exec(argc-1, argv+1);

	fprintf(stderr, "Command \"%s\" is unknown, try \"ip vrf help\".\n", *argv);
	exit(-1);
}
