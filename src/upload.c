#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <err.h>
#include <string.h>
#include <stdlib.h>

#include <curl/curl.h>

#include "upload.h"
#include "replace_str.h"
#include "compat.h"

static size_t	set_location(char *, size_t, size_t, char **);

/*
 * POST the JSON to the new status URL, with a Content-Type of
 * application/json. Do not send the Expect: header, as this fails.
 *
 * First it escapes newlines in the JSON.
 */
char *
upload(char *json, int use_https, char *server_name)
{
	CURL			*handle;
	struct curl_slist	*headers;
	char			*esc_json, errbuf[CURL_ERROR_SIZE];
	char			*content_type, *url, *location;
	int			 len;

	headers = NULL;
	content_type = "Content-Type: application/json";
	location = NULL;

	if (server_name == NULL)
		server_name = DEFAULT_SERVER_NAME;

	/* protocol + "://" + domain + "/" + statuses" + \0 */
	len = (use_https ? 5 : 4) + 3 + strlen(server_name) +
	    1 + strlen(POST_STATUS) + 1;

	if ((url = calloc(len, sizeof(char))) == NULL)
		err(1, "calloc");

	snprintf(url, len,  "%s://%s/%s", use_https ? "https" : "http",
	    server_name, POST_STATUS);

	if ((headers = curl_slist_append(headers, content_type)) == NULL)
		errx(1, "curl_slist_append");

	if ((headers = curl_slist_append(headers, "Expect:")) == NULL)
		errx(1, "curl_slist_append");

	if ((handle = curl_easy_init()) == NULL)
		errx(1, "curl_easy_init failed");

	esc_json = replace_str(json, "\n", "\\n");

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_POSTFIELDS, esc_json);
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, &errbuf);
	curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, set_location);
	curl_easy_setopt(handle, CURLOPT_HEADERDATA, &location);
	curl_easy_setopt(handle, CURLOPT_VERBOSE, 0);

	if (curl_easy_perform(handle) != 0)
		errx(2, "curl: %s", errbuf);

	free(url);
	free(esc_json);
	curl_easy_cleanup(handle);
	curl_slist_free_all(headers);

	return location;
}

static size_t
set_location(char *buffer, size_t size, size_t nitems, char **location)
{
	char	 key[] = "Location: ";
	int	 len, key_len = 10;

	len = nitems - key_len;

	if (strncmp(buffer, key, key_len) != 0)
		return nitems;

	if ((*location = calloc(nitems - key_len, size)) == NULL)
		err(1, "calloc");

	strlcpy(*location, buffer + key_len, len);

	return nitems;
}
