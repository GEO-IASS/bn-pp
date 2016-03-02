#include "variable.hh"

namespace bn {

Variable::Variable(unsigned id, unsigned size)
{
	_id = id;
	_size = size;
}

std::ostream&
operator<<(std::ostream &o, const Variable &v)
{
    o << "Variable(id:" << v._id << ", size:" << v._size << ")";
    return o;
}

}