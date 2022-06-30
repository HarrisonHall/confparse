// confparse.hpp

#include <iostream>

#include "../confparse.hpp"


int main() {
  confparse::Config *conf = confparse::from_file("example.conf");

  std::cout << "example exists: " << conf->has_key("example") << std::endl;
  std::cout << "example.foo: " << conf->get_string("example.foo") << std::endl;
  std::cout << "other.mystring: " << conf->get_string("other.mystring") << std::endl;
  std::cout << "other.mybool: " << conf->get_bool("other.mybool") << std::endl;

  confparse::Subsection *ns = conf->get_subsection("example.sub");
  std::cout << "example.sub.key: " << ns->get_string("key");

  std::cout << "other.myint: " << conf->get_int("other.myint") << std::endl;

  delete ns;
  delete conf;
}
