// confparse.h

#pragma once

#include <errno.h>
#include <inttypes.h>
#include <regex.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// Interface

struct confparse_config;  // Config context
struct confparse_subsection;  // Subsection subtree
struct confparse_value;  // Key-value pair

// Parse config context from filename
struct confparse_config *confparse_parse_file(const char *filename);
// Parse config context from string
struct confparse_config *confparse_parse_string(const char *data);
// Free confparse-allocated structures
void confparse_free(void *type);

// Check if key exists in config/subsection
bool confparse_has_key(void *type, const char *key);

// Get sub-subsection from config/subsection
struct confparse_subsection *confparse_get_subsection(void *type, const char *key);

// Get bool from key in config/subsection
bool confparse_get_bool(void *type, const char *key);
// Get int from key in config/subsection
int64_t confparse_get_int(void *type, const char *key);
// Get float from key in config/subsection
double confparse_get_float(void *type, const char *key);
// Get string from key in config/subsection
const char *confparse_get_string(void *type, const char *key);

// Implementation

enum CONFPARSE_TYPE {CONFIG = 0, SUBSECTION, VALUE};

struct confparse_config {
  enum CONFPARSE_TYPE type;
  size_t num_entries;
  struct confparse_value **values;
};

struct confparse_subsection {
  enum CONFPARSE_TYPE type;
  char *prepended_key;
  struct confparse_config *config;
};

struct confparse_value {
  enum CONFPARSE_TYPE type;
  char *key;
  char *data;
};

enum CONFPARSE_ERROR { NO_ERROR, BAD_MALLOC, NOT_A_FILE };
int cp_errno = NO_ERROR;
const char *cp_errmsg = "";

// Combine subsection with subkey
char *cp_total_key(const char *subsection, const char *key);
// Check if child_subsection is actually a child subsection of parent_subsection
bool cp_is_child_subsection(const char *parent_subsection, const char *child_subsection);
// Find number of configuration lines in file
size_t cp_num_lines_in_config(const char *file);
// Check if file line is a subsection
bool cp_line_matches_subsection(const char *line);
// Check if file line is a key-value pair
bool cp_line_matches_key_value(const char *line);
// Copy subsection from line
char *cp_parse_subsection(const char *line);
// Copy key from line
char *cp_parse_key(const char *line);
// Copy Value from line
char *cp_parse_value(const char *line);
// Get value struct from config
void *cp_find_value(void *type, const char *key);
// Set error and free variables, returns NULL
void *cp_error_out(enum CONFPARSE_ERROR e, const char *message, ...);
// Clear error message and string
void cp_error_clear();


struct confparse_config *confparse_parse_file(const char *filename) {
  cp_error_clear();
  if (access(filename, F_OK) == 0) {
    FILE *filep = fopen(filename, "r");
    fseek(filep, 0, SEEK_END);
    size_t fsize = ftell(filep);
    rewind(filep);
    char *data = (char *)malloc(fsize + 1);
    if (!data) {
      fclose(filep);
      return (struct confparse_config *)cp_error_out(BAD_MALLOC, "Unable to malloc for configuration file");
    }
    fread(data, fsize, 1, filep);
    data[fsize] = '\0';
    fclose(filep);
    struct confparse_config *config = confparse_parse_string(data);
    free(data);
    return config;
  }
  return (struct confparse_config *)cp_error_out(NOT_A_FILE, "File does not exist");
}

struct confparse_config *confparse_parse_string(const char *data) {
  cp_error_clear();
  struct confparse_config *config = (struct confparse_config *)malloc(sizeof(struct confparse_config));
  if (!config) return (struct confparse_config *)cp_error_out(BAD_MALLOC, "Unable to malloc for config struct");
  size_t table_size = cp_num_lines_in_config(data);
  size_t index = 0;
  *config = (struct confparse_config) { CONFIG, table_size, (struct confparse_value **)calloc(table_size, sizeof(struct confparse_value)) };
  if (!config->values) return (struct confparse_config *)cp_error_out(BAD_MALLOC, "Unable to malloc config table", config);
  memset(config->values, 0, table_size * sizeof(struct confparse_value));
  const char *current_subsection = "";
  const char *cursor = data;

  while (index < config->num_entries && *cursor != '\0') {
    if (cp_line_matches_subsection(cursor)) {
      char *new_subsection = cp_parse_subsection(cursor);
      struct confparse_subsection *ns = (struct confparse_subsection *)malloc(sizeof(struct confparse_subsection));
      if (!ns) return (struct confparse_config *)cp_error_out(BAD_MALLOC, "Unable to malloc subsection", config);
      *ns = (struct confparse_subsection) { SUBSECTION, new_subsection, config };
      config->values[index] = (struct confparse_value *)ns;
      index++;
      current_subsection = ns->prepended_key;
    } else if (cp_line_matches_key_value(cursor)) {
      struct confparse_value *v = (struct confparse_value *)malloc(sizeof(struct confparse_value));
      if (!v) return (struct confparse_config *)cp_error_out(BAD_MALLOC, "Unable to malloc value", config);
      char *key = cp_parse_key(cursor);
      char *value = cp_parse_value(cursor);
      *v = (struct confparse_value) { VALUE, cp_total_key(current_subsection, key), value };
      config->values[index] = v;
      index++;
      free(key);
    }
    cursor = strchr(cursor, '\n') + 1;  // Advance to next line
    //if (*(cursor - 1) == '\0') break;  // TODO - fix this
  }
  return config;
}

void confparse_free(void *type) {
  cp_error_clear();
  if (type == NULL) return;
  if (*(enum CONFPARSE_TYPE *)type == CONFIG) {
    struct confparse_config *config = (struct confparse_config *)type;
    for (size_t i = 0; i < config->num_entries; i++) {
      confparse_free(config->values[i]);
    }
    free(config->values);
  } else if (*(enum CONFPARSE_TYPE *)type == SUBSECTION) {
    struct confparse_subsection *subsection = (struct confparse_subsection *)type;
    free(subsection->prepended_key);
  } else if (*(enum CONFPARSE_TYPE *)type == VALUE) {
    struct confparse_value *value = (struct confparse_value *)type;
    free(value->key);
    free(value->data);
  }
  free(type);
}

bool confparse_has_key(void *type, const char *key) {
  cp_error_clear();
  struct confparse_config *config;
  bool found = false;
  if (*(enum CONFPARSE_TYPE *)type == CONFIG) {
    config = (struct confparse_config *)type;
    for (size_t i = 0; i < config->num_entries; i++)
      if (strcmp(key, config->values[i]->key) == 0)
        found = true;
  } else if (*(enum CONFPARSE_TYPE *)type == SUBSECTION) {
    struct confparse_subsection *subsection = (struct confparse_subsection *)type;
    config = subsection->config;
    char *total_key = cp_total_key(subsection->prepended_key, key);
    for (size_t i = 0; i < config->num_entries; i++)
      if (strcmp(total_key, config->values[i]->key) == 0)
        found = true;
    free(total_key);
  }
  return found;
}

struct confparse_subsection *confparse_get_subsection(void *type, const char *key) {
  cp_error_clear();
  struct confparse_subsection *subsection = (struct confparse_subsection *)cp_find_value(type, key);
  if (subsection == NULL) return NULL;
  if (subsection->type != SUBSECTION) return NULL;
  return subsection;
}

bool confparse_get_bool(void *type, const char *key) {
  cp_error_clear();
  struct confparse_value *raw_value = (struct confparse_value*)cp_find_value(type, key);
  if (raw_value == NULL) return false;
  return (strcmp("true", raw_value->data) == 0);
}

int64_t confparse_get_int(void *type, const char *key) {
  cp_error_clear();
  struct confparse_value *raw_value = (struct confparse_value*)cp_find_value(type, key);
  if (raw_value == NULL) return 0;
  int64_t value = 0;
  sscanf(raw_value->data, "%" PRId64, &value);
  return value;
}

double confparse_get_float(void *type, const char *key) {
  cp_error_clear();
  struct confparse_value *raw_value = (struct confparse_value*)cp_find_value(type, key);
  if (raw_value == NULL) return 0;
  double value = 0.0;
  sscanf(raw_value->data, "%lf", &value);
  return value;
}

const char *confparse_get_string(void *type, const char *key) {
  cp_error_clear();
  struct confparse_value *raw_value = (struct confparse_value*)cp_find_value(type, key);
  if (raw_value == NULL) return NULL;
  return (const char *)raw_value->data;
}

char *cp_total_key(const char *subsection, const char *key) {
  int bufsize = strlen(subsection) + strlen(key) + 1 + 1;
  char *total_key = (char *)malloc(bufsize);
  if (!total_key) return (char *)cp_error_out(BAD_MALLOC, "Unable to alloc total_key");
  memset(total_key, 0, bufsize);
  strncat(total_key, subsection, bufsize - 1);
  strcat(total_key, ".");
  strncat(total_key, key, bufsize - 1);
  return total_key;
}

bool cp_is_child_subsection(const char *parent_subsection, const char *child_subsection) {
  return (strncmp(parent_subsection, child_subsection, strlen(parent_subsection)) == 0);
}

size_t cp_num_lines_in_config(const char *file) {
  const char *cursor = file;
  size_t num_lines = 0;
  while (*cursor != '\0') {
    if (cp_line_matches_subsection(cursor) || cp_line_matches_key_value(cursor))
      num_lines++;
    cursor = strchr(cursor, '\n') + 1;  // Advance to next line
    if (cursor - 1 == NULL) break;
  }
  return num_lines;
}

#define CP_REGEX_ANY_OPT ".*"
#define CP_REGEX_WHITESPACE_OPT "[ \\t]*"
#define CP_REGEX_TEXT "[a-zA-Z0-9]+"
#define CP_REGEX_TEXT_SUBSECTION "[a-zA-Z0-9\\.]+"
#define CP_REGEX_COMMENT_OPT "[(\\;|\\#).*]*"
#define CP_REGEX_NEWLINE_OPT "(\\r\\n|\\n)*"
#define SUBSECTION_REGEX_STR \
  "^" CP_REGEX_WHITESPACE_OPT "\\[" CP_REGEX_WHITESPACE_OPT             \
  "(" CP_REGEX_TEXT_SUBSECTION ")"                                       \
  CP_REGEX_WHITESPACE_OPT "\\]" CP_REGEX_WHITESPACE_OPT                 \
  CP_REGEX_COMMENT_OPT CP_REGEX_ANY_OPT
#define KEY_VALUE_REGEX_STR \
  "^" CP_REGEX_WHITESPACE_OPT                                           \
  "(" CP_REGEX_TEXT ")"                                                 \
  CP_REGEX_WHITESPACE_OPT "\\=" CP_REGEX_WHITESPACE_OPT                 \
  "\\\"?([^\n\\\"]*)\\\"?"                                              \
  CP_REGEX_WHITESPACE_OPT CP_REGEX_COMMENT_OPT CP_REGEX_ANY_OPT

bool cp_line_matches_subsection(const char *line) {
  regex_t reegex;
  regcomp(&reegex, SUBSECTION_REGEX_STR, REG_EXTENDED);
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

char *cp_parse_subsection(const char *line) {
  regex_t reegex;
  regcomp(&reegex, SUBSECTION_REGEX_STR, REG_EXTENDED | REG_NEWLINE);
  regmatch_t pmatch[2] = {{-1,-1}, {-1, -1}};
  if (regexec(&reegex, line, 2, pmatch, 0) == 0) {
    size_t len = pmatch[1].rm_eo - pmatch[1].rm_so;
    char *match = (char *)malloc(len);
    if (!match) return (char *)cp_error_out(BAD_MALLOC, "Unable to alloc subsection key");
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
    char *match = (char *)malloc(len);
    if (!match) return (char *)cp_error_out(BAD_MALLOC, "Unable to alloc value key");
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
    char *match = (char *)malloc(len);
    if (!match) return (char *)cp_error_out(BAD_MALLOC, "Unable to alloc value value");
    strncpy(match, line + pmatch[2].rm_so, len);
    match[len] = '\0';
    return match;
  }
  return NULL;
}

void *cp_find_value(void *type, const char *key) {
  struct confparse_config *config = (struct confparse_config *)type;
  const char *total_key = key;
  if (*(enum CONFPARSE_TYPE *)type == SUBSECTION) {
    struct confparse_subsection *subsection = (struct confparse_subsection *)type;
    config = subsection->config;
    total_key = cp_total_key(subsection->prepended_key, key);
  }
  struct confparse_value *value = NULL;
  for (size_t i = 0; i < config->num_entries; i++) {
    if (strcmp(total_key, config->values[i]->key) == 0) {
      value = config->values[i];
      break;
    }
  }
  if (key != total_key) free((void *)total_key);
  return value;
}

void *cp_error_out(enum CONFPARSE_ERROR e, const char *message, ...) {
  va_list argp;
  va_start(argp, message);
  void *arg = va_arg(argp, void *);
  while (arg != NULL) {
    printf("Freeing %p\n", arg);
    confparse_free(arg);
  }
  va_end(argp);
  cp_errno = e;
  cp_errmsg = message;
  return NULL;
}

void cp_error_clear() {
  cp_errno = 0;
  cp_errmsg = "";
}

#ifdef DEBUG
void cp_debug_config(struct confparse_config *config) {
  printf("CONFIG | Correct config: %B (%d)\n", config->type == CONFIG, (int)config->type);
  for (size_t i = 0; i < config->num_entries; i++) {
    enum CONFPARSE_TYPE *type = (enum CONFPARSE_TYPE *)config->values[i];
    if (*type == VALUE) {
      struct confparse_value *value = (struct confparse_value *)type;
      printf("%s:`%s` VALUE | Correct value: %B (%d)\n", value->key, value->data, value->type == VALUE, value->type);
    } else if (*type == SUBSECTION) {
      struct confparse_subsection *subsection = (struct confparse_subsection *)type;
      printf("%s SUBSECTION | Correct value: %B (%d)\n", subsection->prepended_key, subsection->type == SUBSECTION, subsection->type);
    } else {
      printf("INVALID TYPE %d\n", *type);
    }
  }
}
 #endif
