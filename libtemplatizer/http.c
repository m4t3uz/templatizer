#include <stdio.h>
#include <string.h> /* strchr() */
#include <stdlib.h> /* getenv() */
#include <templatizer/templatizer.h>

/* based on rfc6265 */
/*
   cookie-header = "Cookie:" OWS cookie-string OWS
   cookie-string = cookie-pair *( ";" SP cookie-pair )
   cookie-pair   = cookie-name "=" cookie-value
*/
int tmpl_parse_cookie_string(void *data, tmpl_kv_callback cb)
{
	char *query, *p, *token;
	char *key, *value;
	size_t key_len, value_len;

	query = getenv("HTTP_COOKIE");
	if (query == NULL)
		return 1;
	p = query;
	for (;;) {
		/* get key's length */
		token = strchr(p, '=');
		if (token == NULL)
			break; /* No key after amp or key has no value. */
		key = p;
		key_len = token - p;
		p += key_len + 1;

		/* get value's length */
		token = strchr(p, ';');
		if (token) {
			/* This is NOT the last value. */
			value = p;
			value_len = token - p;
			p += value_len + 1;
			if (*(p++) != ' ') {
				fputs("missing space after ';' on cookie string\n", stderr);
				return 1;
			}
			cb(data, key, key_len, value, value_len);
		} else {
			/* This is the last value. */
			value_len = strlen(p);
			value = p;
			cb(data, key, key_len, value, value_len);
			p += value_len + 1;
			break;
		}
	}
	return 0;
}

void tmpl_parse_query_string(char *query, void *data, tmpl_kv_callback cb)
{
	char *p, *token;
	char *key, *value;
	size_t key_len, value_len;

	p = query;
	for (;;) {
		/* get key's length */
		token = strchr(p, '=');
		if (token == NULL)
			break; /* No key after amp or key has no value. */
		key = p;
		key_len = token - p;
		p += key_len + 1;

		/* get value's length */
		token = strchr(p, '&');
		if (token) {
			/* This is NOT the last value. */
			value_len = token - p;
			value = p;
			cb(data, key, key_len, value, value_len);
		} else {
			/* This is the last value. */
			value_len = strlen(p);
			value = p;
			cb(data, key, key_len, value, value_len);
			break;
		}
		p += value_len + 1;
	}
}

int tmpl_parse_query_string_get(void *data, tmpl_kv_callback cb)
{
	char *query;

	query = getenv("QUERY_STRING");
	if (query == NULL)
		return 1;
	tmpl_parse_query_string(query, data, cb);
	return 0;
}

int tmpl_parse_query_string_post(void *data, tmpl_kv_callback cb)
{
	char *content_length, *query;
	int len;

	content_length = getenv("CONTENT_LENGTH");
	if (content_length == NULL)
		return 1;
	len = atoi(content_length);
	query = malloc(len + 1);
	if (query == NULL)
		return 1;
	query[len] = '\0';
	if (fread(query, len, 1, stdin) != 1) {
		free(query);
		return 1;
	}
	fprintf(stderr, "post: query: %s\n", query);
	tmpl_parse_query_string(query, data, cb);
	free(query);
	return 0;
}

void tmpl_throw_error(const char *status, const char *url, const char *msg)
{
	printf("Status: 200\r\n"); /* redirect without changing the url */
	printf("Location: %s?status=%s&%s\r\n", url, status, msg);
	printf("\r\n");
	exit(0);
}

void tmpl_alloc_query(void *query, char *key, int key_len, char *value, int value_len)
{
	unsigned int i;
	struct tmpl_query q = *((struct tmpl_query *) query);

	for (i = 0; i < q.num_params; i++)
		if (strncmp(q.keys[i], key, key_len) == 0)
			if (q.values[i] == NULL)
				q.values[i] = strndup(value, value_len);
}

void tmpl_free_query(void *query, char *key, int key_len, char *value, int value_len)
{
	unsigned int i;
	struct tmpl_query q = *((struct tmpl_query *) query);

	for (i = 0; i < q.num_params; i++)
		if (q.values[i] != NULL)
			free(q.values[i]);
}
