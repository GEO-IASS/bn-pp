#ifndef _BN_IO_FILE_H_
#define _BN_IO_FILE_H_

#include <vector>

#include "variable.hh"

namespace bn {

int
read_uai_model(const char *filename, unsigned &order, std::vector<const Variable*> &variables);

}

#endif
