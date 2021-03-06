#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/wait.h>

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "compat.h"

#include "text.h"

static char	*find_editor();
static char	*read_fd(int);
static int	fd_from_filename(char *);

/*
 * Use ($VISUAL || $EDITOR || /bin/ed) to edit a file with the prefix
 * /tmp/mkmnfst. If the editor exits abnormally, terminate early.
 */
char *
text_from_editor()
{
	int	 fd, status;
	pid_t	 pid;
	char	*contents, *editor, template[24];

	strlcpy(template, "/tmp/mkmnfst.XXXXXXXXXX", 24);

	if ((fd = mkstemp(template)) == -1)
		err(1, "mktemp");

	switch (pid = fork()) {
	case -1:
		err(1, "fork");
		/* NOTREACHED */
		break;
	case 0:
		editor = find_editor();
		execlp(editor, editor, template, NULL);
		break;
	}

	if (waitpid(pid, &status, 0) == -1)
		err(1, "waitpid");

	if (status == 0) {
		contents = read_fd(fd);
		close(fd);
		if (unlink(template) == -1)
			warn("unlink");
	} else {
		close(fd);
		if (unlink(template) == -1)
			warn("unlink");
		err(2, "bad exit status");
	}

	return contents;
}

/*
 * Try to resolve the filename as passed in by the user, and produce the whole
 * contents of it.
 */
char *
text_from_file(char *argv_filename)
{
	char	*buf;
	int	 fd;

	if ((strcmp(argv_filename, "-")) == 0)
		fd = STDIN_FILENO;
	else
		fd = fd_from_filename(argv_filename);

	buf = read_fd(fd);

	close(fd);

	return buf;
}

/*
 * Read the contents of a file descriptor and produce that as a character
 * pointer.
 */
static char *
read_fd(int fd)
{
	char	*buf, *nbuf;
	int	 nread;
	ssize_t	 ret;
	size_t	 nbytes;

	nbytes = 1024;
	nread = 0;

	if ((buf = calloc(nbytes, sizeof(char))) == NULL)
		err(1, "calloc");

	while ((ret = read(fd, buf + nread, nbytes)) > 0) {
		nread += nbytes;
		if ((nbuf = reallocarray(buf, nread + nbytes, sizeof(char))) == NULL)
			err(1, "reallocarray");

		buf = nbuf;
	}

	if (ret == -1)
		err(1, "read");

	return buf;
}

/*
 * Produce $VISUAL, $EDITOR, or /bin/ed, in that order.
 */
static char *
find_editor()
{
	char *editor;

	editor = getenv("VISUAL");

	if (editor == NULL)
		editor = getenv("EDITOR");

	if (editor == NULL)
		editor = "/bin/ed";

	return editor;
}

/*
 * Return a file descriptor for the given filename
 */
static int
fd_from_filename(char *argv_filename)
{
	char	*filename;
	int	 fd;

	if ((filename = realpath(argv_filename, NULL)) == NULL)
		err(1, "readpath");

	if ((fd = open(filename, O_RDONLY)) == -1)
		err(1, "open");

	free(filename);

	return fd;
}
