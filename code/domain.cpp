#include "domain.hh"

#include <iostream>

using namespace std;

namespace bn {

Domain::Domain()
{
    _width = 0;
    _size = 1;
}

Domain::Domain(vector<const Variable*> scope) : _scope(scope), _width(scope.size())
{
    _size = 1;
    if (_width > 0) {
        _offset.reserve(_width);
        for (int i = _width-1; i >= 0; --i) {
            _offset[i] = _size;
            _size *= _scope[i]->size();
            _var_to_index[_scope[i]->id()] = i;
        }
    }
}

Domain::Domain(const Domain &d) : Domain(d._scope)
{
}

Domain::Domain(const Domain &d1, const Domain &d2)
{
    _scope = d1._scope;
    unsigned width2 = d2._width;
    for (unsigned i = 0; i < width2; ++i) {
        const Variable *v = d2[i];
        if (!d1.in_scope(v)) {
            _scope.push_back(v);
        }
    }
    _width = _scope.size();
    _size = 1;
    if (_width > 0) {
        _offset.reserve(_width);
        for (int i = _width-1; i >= 0; --i) {
            _offset[i] = _size;
            _size *= _scope[i]->size();
            _var_to_index[_scope[i]->id()] = i;
        }
    }
}

Domain::Domain(const Domain &d, const Variable *v)
{
    for (unsigned i = 0; i < d._width; ++i) {
        if (d._scope[i] != v) {
            _scope.push_back(d._scope[i]);
        }
    }
    _width = _scope.size();
    _size = 1;
    if (_width > 0) {
        _offset.reserve(_width);
        for (int i = _width-1; i >= 0; --i) {
            const Variable *var = _scope[i];
            _offset[i] = _size;
            _size *= var->size();
            _var_to_index[var->id()] = i;
        }
    }
}

Domain::Domain(const Domain &d, const unordered_map<unsigned,unsigned> &evidence)
{
    for (auto it_scope : d._scope) {
        const Variable *variable = it_scope;
        if (!evidence.count(variable->id())) {
            _scope.push_back(variable);
        }
    }
    _width = _scope.size();
    _size = 1;
    _offset.reserve(_width);
    for (int i = _width-1; i >= 0; --i) {
        _offset[i] = _size;
        _size *= _scope[i]->size();
        _var_to_index[_scope[i]->id()] = i;
    }
}

const Variable*
Domain::operator[](unsigned i) const
{
    if (i < _width) return _scope[i];
    else throw "Domain::operator[unsigned i]: Index out of range!";
}

bool
Domain::in_scope(const Variable* v) const
{
    unordered_map<unsigned,unsigned>::const_iterator it = _var_to_index.find(v->id());
    return (it != _var_to_index.end());
}

bool
Domain::in_scope(unsigned id) const
{
    unordered_map<unsigned,unsigned>::const_iterator it = _var_to_index.find(id);
    return (it != _var_to_index.end());
}

void
Domain::next_valuation(vector<unsigned> &valuation) const
{
    int j;
    for (j = valuation.size()-1; j >= 0 && valuation[j] == _scope[j]->size()-1; --j) {
        valuation[j] = 0;
    }
    if (j >= 0) {
        valuation[j]++;
    }
}

void
Domain::next_valuation_with_evidence(vector<unsigned> &valuation, const unordered_map<unsigned,unsigned> &evidence) const
{
    int j;
    for (j = valuation.size()-1; j >= 0 && (evidence.count(_scope[j]->id()) || valuation[j] == _scope[j]->size()-1); --j) {
        if (evidence.count(_scope[j]->id())) continue;
        valuation[j] = 0;
    }
    if (j >= 0) {
        valuation[j]++;
    }
}

void
Domain::update_valuation_with_evidence(vector<unsigned> &valuation, const unordered_map<unsigned,unsigned> &evidence) const {
    if (_width == 0) return;
    for (auto it_evidence : evidence) {
        unsigned id = it_evidence.first;
        unsigned value = it_evidence.second;

        unordered_map<unsigned,unsigned>::const_iterator it_index = _var_to_index.find(id);
        if (it_index != _var_to_index.end()) {
            valuation[it_index->second] = value;
        }
    }
}

unsigned
Domain::position_valuation(vector<unsigned> valuation) const
{
    unsigned pos = 0;
    for (int i = _width-1; i >= 0; --i) {
        pos += valuation[i] * _offset[i];
    }
    return pos;
}

unsigned
Domain::position_consistent_valuation(vector<unsigned> valuation, const Domain &domain) const
{
    if (_width == 0) { return 0; }
    else {
        unsigned pos = 0;
        unsigned index = 0;
        for (auto v : _scope) {
            unordered_map<unsigned,unsigned>::const_iterator it_index = _var_to_index.find(v->id());
            unordered_map<unsigned,unsigned>::const_iterator it_index2 = domain._var_to_index.find(v->id());
            if (it_index != _var_to_index.end() && it_index2 != _var_to_index.end()) {
                pos +=  _offset[it_index->second] * valuation[it_index2->second];
            }
            index++;
        }
        return pos;
    }
}

unsigned
Domain::position_consistent_valuation(vector<unsigned> valuation, const Domain &domain, const Variable *v, unsigned value) const
{
    unsigned pos = position_consistent_valuation(valuation, domain);
    unordered_map<unsigned,unsigned>::const_iterator it_index = _var_to_index.find(v->id());
    if (it_index != _var_to_index.end()) {
        pos += _offset[it_index->second] * value;
    }
    return pos;
}

ostream&
operator<<(ostream &o, const Domain &d)
{
    unsigned width = d._width;
    if (width == 0) { o << "Domain{}"; }
    else {
        o << "Domain{";
        unsigned i;
        for (i = 0; i < width-1; ++i) {
            o << d._scope[i]->id() << ", ";
        }
        o << d._scope[i]->id() << "}";
    }
    return o;
}

}