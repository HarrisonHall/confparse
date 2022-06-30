# ConfParse
Straightforward header-only ini/conf file parser.

The goal was to be concise and header-only while maintaining readability
and a simple interface. PRs welcome if they fix bugs or align with those
goals.

## Interface
Take directly from [confparse.h](confparse.h).
```c
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
```

## Example Usage
Check out [examples/test.c](examples/test.c). Here's an example.

```c
struct confparse_config *conf = confparse_parse_file("example.conf");

bool myflag = confparse_get_bool(conf, "mysection.myflag");

if (confparse_has_key(conf, "mysection.mysubsection")) {
  struct confparse_subsection *ns = confparse_get_subsection(conf, "mysection.mysubsection");
  int64_t myint = confparse_get_int(ns, "myint");
  double myfloat = confparse_get_float(ns, "myfloat");
  char *mystring = confparse_get_string(ns, "mystring");
}

confparse_free(conf);
```

## Misc.
There is also a basic error system implemented. If something goes wrong when
generating the original config struct, NULL will be returned. If something goes
wrong during config population or interface usage, `cp_errno` and `cp_errmsg` are
set. Note: these are reset during each interface call. 
