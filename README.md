# ConfParse
Straightforward header-only ini/conf file parser.

## Interface
```
#include "confparse.h"


struct confparse_config *config = confparse_parse_file("myfile.conf");

bool mybool = false;
if (confparse_has_key(config, "section.key")) {
  myflag = confparse_get_bool(config, "section.key");
}


struct confparse_namespace *namespace = confparse_get_namespace(config, "section");
if (myflag && namespace) {
  bool myflag2 = confparse_get_bool(namespace, "key");
  int64_t myint = confparse_get_int(namespace, "someval");
  double myfloat = confparse_get_float(namespace, "someval2");
  char *mystring = confparse_get_string(namespace, "someval3");
}
```
