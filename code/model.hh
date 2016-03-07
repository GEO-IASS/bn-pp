#ifndef _BN_MODEL_H_
#define _BN_MODEL_H_

#include "variable.hh"
#include "factor.hh"

#include <string>
#include <vector>
#include <unordered_map>

namespace bn {

class BN {
public:
	BN(std::string name, std::vector<Variable*> &variables, std::vector<Factor*> &factors);
	~BN();

	Factor joint_distribution();

	Factor query(std::vector<Variable*> &target, std::vector<Variable*> &evidence);

	friend std::ostream &operator<<(std::ostream &os, const BN &bn);

private:
	std::string _name;
	std::vector<Variable*> _variables;
	std::vector<Factor*> _factors;
	std::unordered_map<unsigned,std::vector<const Variable*>> _parents;
	std::unordered_map<unsigned,std::vector<const Variable*>> _children;
};

}

#endif