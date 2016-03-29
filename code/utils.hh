#include "model.hh"

#include <string>
#include <unordered_set>

namespace bn {

int
parse_vars_set(const Model *model, const std::string, std::unordered_set<const Variable*> &vars_set);

}

