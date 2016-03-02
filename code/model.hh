#ifndef _BN_MODEL_H_
#define _BN_MODEL_H_

#include "variable.hh"
#include "factor.hh"

#include <string>
#include <vector>

namespace bn {

class BN {
public:
	BN(std::string name, std::vector<Variable*> &variables, std::vector<Factor*> &factors);
	~BN();

	friend std::ostream &operator<<(std::ostream &os, const BN &bn);

private:
	std::string _name;
	std::vector<Variable*> _variables;
	std::vector<Factor*> _factors;
};

}

#endif