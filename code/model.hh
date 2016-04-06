#ifndef _BN_MODEL_H_
#define _BN_MODEL_H_

#include "variable.hh"
#include "factor.hh"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace bn {

class Model {
public:
	Model(std::string name, std::vector<Variable*> &variables, std::vector<Factor*> &factors);
	virtual ~Model();

	const std::string name() const { return _name; };
	const std::vector<Variable*> &variables() const { return _variables; };
	const std::vector<Factor*>   &factors()   const { return _factors;   };

	Factor joint_distribution() const;
	Factor joint_distribution(const std::unordered_map<unsigned,unsigned> &evidence) const;

	virtual double partition(
		const std::unordered_map<unsigned,unsigned> &evidence,
		double &uptime) const;

	virtual std::vector<const Factor*> marginals(
		const std::unordered_map<unsigned,unsigned> &evidence,
		double &uptime) const;

	virtual Factor marginal(
		const Variable *v,
		Factor &joint) const;

	virtual void write(std::ostream&) const = 0;

protected:
	std::string _name;
	std::vector<Variable*> _variables;
	std::vector<Factor*> _factors;
};

class BN : public Model {
public:
	BN(std::string name, std::vector<Variable*> &variables, std::vector<Factor*> &factors);

	const std::unordered_set<const Variable*> parents(const Variable *v)  const { return _parents.find(v)->second;  };
	const std::unordered_set<const Variable*> children(const Variable *v) const { return _children.find(v)->second; };

	double partition(
		const std::unordered_map<unsigned,unsigned> &evidence,
		double &uptime) const;

	std::vector<const Factor*> marginals(
		const std::unordered_map<unsigned,unsigned> &evidence,
		double &uptime) const;

	Factor query(
		const std::unordered_set<const Variable*> &target,
		const std::unordered_set<const Variable*> &evidence,
		std::unordered_map<std::string,bool> &options,
		double &uptime) const;

	Factor query_ve(
		const std::unordered_set<const Variable*> &target,
		const std::unordered_set<const Variable*> &evidence,
		std::unordered_map<std::string,bool> &options,
		double &uptime) const;

	Factor variable_elimination(
		std::vector<const Variable*> &variables,
		std::vector<const Factor*> &factors) const;

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

	std::unordered_map<unsigned,unsigned> sampling() const;

	std::unordered_set<const Variable*> markov_independence(const Variable* v) const;
	std::unordered_set<const Variable*> descendants(const Variable *v) const;

	std::unordered_set<const Variable*> ancestors(const Variable *v) const;
	std::unordered_set<const Variable*> ancestors(const std::unordered_set<const Variable*> &vars) const;

	void write(std::ostream& os) const;
	friend std::ostream &operator<<(std::ostream &os, const BN &bn);

private:
	std::unordered_map<const Variable*,std::unordered_set<const Variable*>> _parents;
	std::unordered_map<const Variable*,std::unordered_set<const Variable*>> _children;

	std::vector<const Factor*> sampling_order() const;
};

class MN  : public Model {
public:
	MN(std::string name, std::vector<Variable*> &variables, std::vector<Factor*> &factors);

	const std::unordered_set<const Variable*> neighbors(const Variable *v)  const { return _neighbors.find(v)->second;  };

	void write(std::ostream& os) const;
	friend std::ostream &operator<<(std::ostream &os, const MN &bn);

private:
	std::unordered_map<const Variable*,std::unordered_set<const Variable*>> _neighbors;
};

}

#endif