// confparse.hpp

#include <new>
#include <string>

namespace confparse {

extern "C" {
  #include "confparse.h"
}

class Config;
class Subsection;

Config *from_file(const std::string &filename);
Config *from_string(const std::string &file);

class Config {
public:
  Config(struct confparse_config *c)
    : c(c) {}
  virtual ~Config() { confparse_free(c); }
  virtual bool has_key(const std::string &key);
  virtual bool get_bool(const std::string &key);
  virtual int64_t get_int(const std::string &key);
  virtual double get_float(const std::string &key);
  virtual std::string get_string(const std::string &key);
  virtual Subsection *get_subsection(const std::string &key);
private:
  struct confparse_config *c = nullptr;
};

class Subsection {
public:
  Subsection(struct confparse_subsection *ns)
    : ns(ns) {}
  virtual ~Subsection() {}
  virtual bool has_key(const std::string &key);
  virtual bool get_bool(const std::string &key);
  virtual int64_t get_int(const std::string &key);
  virtual double get_float(const std::string &key);
  virtual std::string get_string(const std::string &key);
  virtual Subsection *get_subsection(const std::string &key);
private:
  struct confparse_subsection *ns = nullptr;
};

Config *from_file(const std::string &filename) {
  struct confparse_config *c = confparse_parse_file(filename.c_str());
  if (c == NULL) throw std::bad_alloc();
  return new Config(c);
}
Config *from_string(const std::string &file) {
  struct confparse_config *c = confparse_parse_string(file.c_str());
  if (c == NULL) throw std::bad_alloc();
  return new Config(c);
}

bool Config::has_key(const std::string &key) {
  return confparse_has_key(c, key.c_str());
}
bool Config::get_bool(const std::string &key) {
  return confparse_get_bool(c, key.c_str());
}
int64_t Config::get_int(const std::string &key) {
  return confparse_get_int(c, key.c_str());
}
double Config::get_float(const std::string &key) {
  return confparse_get_float(c, key.c_str());
}
std::string Config::get_string(const std::string &key) {
  return std::string(confparse_get_string(c, key.c_str()));
}
Subsection *Config::get_subsection(const std::string &key) {
  return new Subsection(confparse_get_subsection(c, key.c_str()));
}

bool Subsection::has_key(const std::string &key) {
  return confparse_has_key(ns, key.c_str());
}
bool Subsection::get_bool(const std::string &key) {
  return confparse_get_bool(ns, key.c_str());
}
int64_t Subsection::get_int(const std::string &key) {
  return confparse_get_int(ns, key.c_str());
}
double Subsection::get_float(const std::string &key) {
  return confparse_get_float(ns, key.c_str());
}
std::string Subsection::get_string(const std::string &key) {
  return std::string(confparse_get_string(ns, key.c_str()));
}
Subsection *Subsection::get_subsection(const std::string &key) {
  return new Subsection(confparse_get_subsection(ns, key.c_str()));
}

}
