/* Copyright 2014 Mike Burns, mnfst */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "compat.h"

#include "sign.h"
#include "text.h"
#include "upload.h"

__dead void	usage();
static void	sign_and_upload(char *, char *, int, char *);

extern char	*optarg;

/*
 * Post a manifest to Mnfst.
 *
 * Usage:
 *
 *   mkmnfst [file]
 */
int
main(int argc, char *argv[])
{
	int	 ch;
	char	*text, *server_name, *keyid;
	int	 use_https;

	server_name = NULL;
	use_https = 1;
	text = NULL;
	keyid = NULL;

	while ((ch = getopt(argc, argv, "0r:s:")) != -1)
		switch (ch) {
		case '0':
			use_https = 0;
			break;
		case 'r':
			keyid = optarg;
			break;
		case 's':
			server_name = optarg;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	switch (argc) {
	case 0:
		text = text_from_editor();
		break;
	case 1:
		text = text_from_file(argv[0]);
		break;
	default:
		usage();
		/* NOTREACHED */
		break;
	}

	sign_and_upload(text, keyid, use_https, server_name);

	free(text);

	return 0;
}

/*
 * Display a usage message and exit.
 */
void
usage()
{
	fprintf(stderr, "usage: mkmnfst [-0] [-s server] [-r keyid] [file]\n");
	exit(64);
}

/*
 * Sign the text, stick it in a JSON, and upload the JSON.
 */
static void
sign_and_upload(char *text, char *keyid, int use_https, char *server_name)
{
	char	*signature, *json, *json_template, *location;
	int	 len;

	json_template = "{\"status\":{\"signed_body\":\"%s\"}}";
	location = NULL;

	signature = sign(text, keyid);

	len = strlen(signature) + 30;
	if ((json = calloc(len, sizeof(char))) == NULL)
		err(1, "calloc");

	snprintf(json, len, json_template, signature);
	free(signature);

	location = upload(json, use_https, server_name);

	printf("%s\n", location);

	free(json);
	free(location);
}
