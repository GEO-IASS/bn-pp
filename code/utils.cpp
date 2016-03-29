#include "utils.hh"

#include <regex>
using namespace std;

namespace bn {

int
parse_vars_set(const Model *model, const std::string s, std::unordered_set<const Variable*> &vars_set)
{
	regex vars_set_regex("[0-9]+(,[0-9]+)*");
	if (!regex_match(s, vars_set_regex)) {
		return -1;
	}

	string var = "";
	for (const char& c : s) {
		if (c != ',') var += c;
		else {
			vars_set.insert(model->variables()[stoi(var)]);
			var = "";
		}
	}
	vars_set.insert(model->variables()[stoi(var)]);

	return 0;
}

}