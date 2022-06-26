
#include <stdio.h>

#include "../confparse.h"


int main() {
  char filename[] = "example.conf";
  struct confparse_config *conf = confparse_parse_file(filename);

  printf("%s exists: %B\n", "example", confparse_has_key(conf, "example"));
  printf("%s exists: %B\n", "example.sub", confparse_has_key(conf, "example.sub"));
  printf("%s exists: %B\n", "other", confparse_has_key(conf, "other"));
  printf("%s exists: %B\n", "other.sub", confparse_has_key(conf, "other.sub"));

  printf("%s exists: %B\n", "example.foo", confparse_has_key(conf, "example.foo"));
  printf("example.foo %s\n", confparse_get_string(conf, "example.foo"));
  printf("other.mystring %s\n", confparse_get_string(conf, "other.mystring"));
  printf("%f\n", confparse_get_float(conf, "other.mybool"));

  struct confparse_namespace *example = confparse_get_namespace(conf, "example");
  printf("%p\n", example);
  printf("passed %d\n", example->type);
  printf("example.sub.key: %s\n", confparse_get_string(example, "key"));
  printf("other.myint: %d\n", (int)confparse_get_int(conf, "other.myint"));

#ifdef DEBUG
  printf("\n\nDEBUG\n");
  cp_debug_config(conf);
#endif
  
  free(conf);
}
