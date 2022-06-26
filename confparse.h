// confparse.h

#include <inttypes.h>
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// Interface

struct confparse_config;  // Config context
struct confparse_namespace;  // Namespace subtree
struct confparse_value;  // Key-value pair

// Parse config context from filename
struct confparse_config *confparse_parse_file(const char *filename);
// Parse config context from string
struct confparse_config *confparse_parse_string(const char *data);

// Check if key exists in config/namespace
bool confparse_has_key(void *type, const char *key);

// Get sub-namespace from config/namespace
struct confparse_namespace *confparse_get_namespace(void *type, const char *key);

// Get bool from key in config/namespace
bool confparse_get_bool(void *type, const char *key);
// Get int from key in config/namespace
int64_t confparse_get_int(void *type, const char *key);
// Get float from key in config/namespace
double confparse_get_float(void *type, const char *key);
// Get string from key in config/namespace
const char *confparse_get_string(void *type, const char *key);


// Implementation

enum CONFPARSE_TYPE {CONFIG = 0, NAMESPACE, VALUE};

struct confparse_config {
  enum CONFPARSE_TYPE type;
  size_t num_entries;
  struct confparse_value **values;
};

struct confparse_namespace {
  enum CONFPARSE_TYPE type;
  char *prepended_key;
  struct confparse_config *config;
};

struct confparse_value {
  enum CONFPARSE_TYPE type;
  char *key;
  char *data;
};

// Combine namespace with subkey
char *cp_total_key(const char *namespace, const char *key);
// Check if child_namespace is actually a child namespace of parent_namespace
bool cp_is_child_namespace(const char *parent_namespace, const char *child_namespace);
// Find number of configuration lines in file
size_t cp_num_lines_in_config(const char *file);
// Check if file line is a namespace
bool cp_line_matches_namespace(const char *line);
// Check if file line is a key-value pair
bool cp_line_matches_key_value(const char *line);
// Copy namespace from line
char *cp_parse_namespace(const char *line);
// Copy key from line
char *cp_parse_key(const char *line);
// Copy Value from line
char *cp_parse_value(const char *line);
// Get value struct from config
void *cp_find_value(void *type, const char *key);


struct confparse_config *confparse_parse_file(const char *filename) {
  if (access(filename, F_OK) == 0) {
    FILE *filep = fopen(filename, "r");
    fseek(filep, 0, SEEK_END);
    size_t fsize = ftell(filep);
    rewind(filep);
    char *data = malloc(fsize + 1);
    fread(data, fsize, 1, filep);
    data[fsize] = '\0';
    fclose(filep);
    struct confparse_config *config = confparse_parse_string(data);
    free(data);
    return config;
  }
  return NULL;
}

struct confparse_config *confparse_parse_string(const char *data) {
  struct confparse_config *config = malloc(sizeof(struct confparse_config));
  size_t table_size = cp_num_lines_in_config(data);
  size_t index = 0;
  *config = (struct confparse_config) { CONFIG, table_size, calloc(table_size, sizeof(struct confparse_value)) };
  const char *current_namespace = "";
  const char *cursor = data;

  while (index < config->num_entries && *cursor != '\0') {
    if (cp_line_matches_namespace(cursor)) {
      char *new_namespace = cp_parse_namespace(cursor);
      struct confparse_namespace *ns = malloc(sizeof(struct confparse_namespace));
      *ns = (struct confparse_namespace) { NAMESPACE, new_namespace, config };
      config->values[index] = (struct confparse_value *)ns;
      index++;
      current_namespace = ns->prepended_key;
    } else if (cp_line_matches_key_value(cursor)) {
      struct confparse_value *v = malloc(sizeof(struct confparse_value));
      char *key = cp_parse_key(cursor);
      char *value = cp_parse_value(cursor);
      *v = (struct confparse_value) { VALUE, cp_total_key(current_namespace, key), value };
      config->values[index] = v;
      index++;
      free(key);
    }
    cursor = strchr(cursor, '\n') + 1;  // Advance to next line
    if (cursor - 1 == NULL) break;
  }
  return config;
}

bool confparse_has_key(void *type, const char *key) {
  struct confparse_config *config;
  bool found = false;
  if (*(enum CONFPARSE_TYPE *)type == CONFIG) {
    config = (struct confparse_config *)type;
    for (size_t i = 0; i < config->num_entries; i++)
      if (strcmp(key, config->values[i]->key) == 0)
        found = true;
  } else if (*(enum CONFPARSE_TYPE *)type == NAMESPACE) {
    struct confparse_namespace *namespace = (struct confparse_namespace *)type;
    config = namespace->config;
    char *total_key = cp_total_key(namespace->prepended_key, key);
    for (size_t i = 0; i < config->num_entries; i++)
      if (strcmp(total_key, config->values[i]->key) == 0)
        found = true;
    free(total_key);
  }
  return found;
}

struct confparse_namespace *confparse_get_namespace(void *type, const char *key) {
  struct confparse_namespace *namespace = (struct confparse_namespace *)cp_find_value(type, key);
  if (namespace == NULL) return NULL;
  if (namespace->type != NAMESPACE) return NULL;
  return namespace;
}

bool confparse_get_bool(void *type, const char *key) {
  struct confparse_value *raw_value = (struct confparse_value*)cp_find_value(type, key);
  if (raw_value == NULL) return false;
  return (strcmp("true", raw_value->data) == 0);
}

int64_t confparse_get_int(void *type, const char *key) {
  struct confparse_value *raw_value = (struct confparse_value*)cp_find_value(type, key);
  if (raw_value == NULL) return 0;
  int64_t value = 0;
  sscanf(raw_value->data, "%" PRId64, &value);
  return value;
}

double confparse_get_float(void *type, const char *key) {
  struct confparse_value *raw_value = (struct confparse_value*)cp_find_value(type, key);
  if (raw_value == NULL) return 0;
  double value = 0.0;
  sscanf(raw_value->data, "%lf", &value);
  return value;
}

const char *confparse_get_string(void *type, const char *key) {
  struct confparse_value *raw_value = (struct confparse_value*)cp_find_value(type, key);
  if (raw_value == NULL) return NULL;
  return (const char *)raw_value->data;
}

char *cp_total_key(const char *namespace, const char *key) {
  int bufsize = strlen(namespace) + strlen(key) + 1 + 1;
  char *total_key = malloc(bufsize);
  memset(total_key, 0, bufsize);
  strncat(total_key, namespace, bufsize - 1);
  strcat(total_key, ".");
  strncat(total_key, key, bufsize - 1);
  return total_key;
}

bool cp_is_child_namespace(const char *parent_namespace, const char *child_namespace) {
  return (strncmp(parent_namespace, child_namespace, strlen(parent_namespace)) == 0);
}

size_t cp_num_lines_in_config(const char *file) {
  const char *cursor = file;
  size_t num_lines = 0;
  while (*cursor != '\0') {
    if (cp_line_matches_namespace(cursor) || cp_line_matches_key_value(cursor))
      num_lines++;
    cursor = strchr(cursor, '\n') + 1;  // Advance to next line
    if (cursor - 1 == NULL) break;
  }
  return num_lines;
}

#define CP_REGEX_ANY_OPT ".*"
#define CP_REGEX_WHITESPACE_OPT "[ \\t]*"
#define CP_REGEX_TEXT "[a-zA-Z0-9]+"
#define CP_REGEX_TEXT_NAMESPACE "[a-zA-Z0-9\\.]+"
//#define CP_REGEX_QUOTED_STRING "((\\\")|[^\"(\\\")])+"
//#define CP_REGEX_QUOTED_STRING "\\\"?.*\\\"?"
#define CP_REGEX_COMMENT_OPT "[(\\;|\\#).*]*"
#define CP_REGEX_NEWLINE_OPT "(\\r\\n|\\n)*"
#define NAMESPACE_REGEX_STR \
  "^" CP_REGEX_WHITESPACE_OPT "\\[" CP_REGEX_WHITESPACE_OPT             \
  "(" CP_REGEX_TEXT_NAMESPACE ")"                                       \
  CP_REGEX_WHITESPACE_OPT "\\]" CP_REGEX_WHITESPACE_OPT                 \
  CP_REGEX_COMMENT_OPT CP_REGEX_ANY_OPT
#define KEY_VALUE_REGEX_STR \
  "^" CP_REGEX_WHITESPACE_OPT                                           \
  "(" CP_REGEX_TEXT ")"                                                 \
  CP_REGEX_WHITESPACE_OPT "\\=" CP_REGEX_WHITESPACE_OPT                 \
  "\\\"?([^\n\\\"]*)\\\"?"                                       \
  CP_REGEX_WHITESPACE_OPT CP_REGEX_COMMENT_OPT CP_REGEX_ANY_OPT

bool cp_line_matches_namespace(const char *line) {
  regex_t reegex;
  regcomp(&reegex, NAMESPACE_REGEX_STR, REG_EXTENDED);
  regmatch_t pmatch[2] = {{-1,-1}, {-1, -1}};
  if (regexec(&reegex, line, 2, pmatch, 0) == 0) {
    return true;
  }
  return false;
}

bool cp_line_matches_key_value(const char *line) {
  regex_t reegex;
  regcomp(&reegex, KEY_VALUE_REGEX_STR, REG_EXTENDED | REG_NEWLINE);
  regmatch_t pmatch[2] = {{-1,-1}, {-1, -1}};
  if (regexec(&reegex, line, 2, pmatch, 0) == 0) {
    return true;
  }
  return false;
}

char *cp_parse_namespace(const char *line) {
  regex_t reegex;
  regcomp(&reegex, NAMESPACE_REGEX_STR, REG_EXTENDED | REG_NEWLINE);
  regmatch_t pmatch[2] = {{-1,-1}, {-1, -1}};
  if (regexec(&reegex, line, 2, pmatch, 0) == 0) {
    size_t len = pmatch[1].rm_eo - pmatch[1].rm_so;
    char *match = malloc(len);
    strncpy(match, line + pmatch[1].rm_so, len);
    match[len] = '\0';
    return match;
  }
  return NULL;
}

char *cp_parse_key(const char *line) {
  regex_t reegex;
  regcomp(&reegex, KEY_VALUE_REGEX_STR, REG_EXTENDED | REG_NEWLINE);
  regmatch_t pmatch[3] = {{-1,-1}, {-1, -1}, {-1, -1}};
  if (regexec(&reegex, line, 3, pmatch, 0) == 0) {
    size_t len = pmatch[1].rm_eo - pmatch[1].rm_so;
    char *match = malloc(len);
    strncpy(match, line + pmatch[1].rm_so, len);
    match[len] = '\0';
    return match;
  }
  return NULL;
}

char *cp_parse_value(const char *line) {
  regex_t reegex;
  regcomp(&reegex, KEY_VALUE_REGEX_STR, REG_EXTENDED | REG_NEWLINE);
  regmatch_t pmatch[3] = {{-1,-1}, {-1, -1}, {-1, -1}};
  if (regexec(&reegex, line, 3, pmatch, 0) == 0) {
    size_t len = pmatch[2].rm_eo - pmatch[2].rm_so;
    char *match = malloc(len);
    strncpy(match, line + pmatch[2].rm_so, len);
    match[len] = '\0';
    return match;
  }
  return NULL;
}

void *cp_find_value(void *type, const char *key) {
  struct confparse_config *config = (struct confparse_config *)type;
  const char *total_key = key;
  if (*(enum CONFPARSE_TYPE *)type == NAMESPACE) {
    struct confparse_namespace *namespace = (struct confparse_namespace *)type;
    config = namespace->config;
    total_key = cp_total_key(namespace->prepended_key, key);
  }
  struct confparse_value *value = NULL;
  for (size_t i = 0; i < config->num_entries; i++) {
    if (strcmp(total_key, config->values[i]->key) == 0) {
      printf("Matches: %s-%s\n", total_key, config->values[i]->key);
      value = config->values[i];
      break;
    }
      
  }
  if (key != total_key) free((void *)total_key);
  return value;
}

#ifdef DEBUG
void cp_debug_config(struct confparse_config *config) {
  printf("CONFIG | Correct config: %B (%d)\n", config->type == CONFIG, (int)config->type);
  for (size_t i = 0; i < config->num_entries; i++) {
    enum CONFPARSE_TYPE *type = (enum CONFPARSE_TYPE *)config->values[i];
    if (*type == VALUE) {
      struct confparse_value *value = (struct confparse_value *)type;
      printf("%s:`%s` VALUE | Correct value: %B (%d)\n", value->key, value->data, value->type == VALUE, value->type);
    } else if (*type == NAMESPACE) {
      struct confparse_namespace *namespace = (struct confparse_namespace *)type;
      printf("%s NAMESPACE | Correct value: %B (%d)\n", namespace->prepended_key, namespace->type == NAMESPACE, namespace->type);
    } else {
      printf("INVALID TYPE %d\n", *type);
    }
  }
}
#endif
