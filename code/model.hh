#ifndef _BN_MODEL_H_
#define _BN_MODEL_H_

#include "variable.hh"
#include "factor.hh"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace bn {

class BN {
public:
	BN(std::string name, std::vector<Variable*> &variables, std::vector<Factor*> &factors);
	~BN();

	const std::vector<Variable*> &variables() const { return _variables; };
	const std::vector<Factor*>   &factors()   const { return _factors;   };

	const std::unordered_set<const Variable*> parents(const Variable *v)  const { return _parents.find(v)->second;  };
	const std::unordered_set<const Variable*> children(const Variable *v) const { return _children.find(v)->second; };

	Factor joint_distribution() const;
	Factor query(
		const std::unordered_set<const Variable*> &target,
		const std::unordered_set<const Variable*> &evidence,
		double &uptime,
		std::unordered_map<std::string,bool> &options) const;

	void bayes_ball(
		const std::unordered_set<const Variable*> &J,
		const std::unordered_set<const Variable*> &K,
		const std::unordered_set<const Variable*> &F,
		std::unordered_set<const Variable*> &Np,
		std::unordered_set<const Variable*> &Ne) const;

	bool m_separated(
		const Variable *v1, const Variable *v2,
		const std::unordered_set<const Variable*> evidence,
		bool verbose=false) const;

	std::unordered_set<const Variable*> markov_independence(const Variable* v) const;
	std::unordered_set<const Variable*> descendants(const Variable *v) const;

	std::unordered_set<const Variable*> ancestors(const Variable *v) const;
	std::unordered_set<const Variable*> ancestors(const std::unordered_set<const Variable*> &vars) const;

	friend std::ostream &operator<<(std::ostream &os, const BN &bn);

private:
	std::string _name;
	std::vector<Variable*> _variables;
	std::vector<Factor*> _factors;
	std::unordered_map<const Variable*,std::unordered_set<const Variable*>> _parents;
	std::unordered_map<const Variable*,std::unordered_set<const Variable*>> _children;
};

}

#endif