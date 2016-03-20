#ifndef _BN_IO_FILE_H_
#define _BN_IO_FILE_H_

#include <string>

#include "model.hh"

namespace bn {

std::string
read_uai_model(const char *filename, unsigned &order, Model **model);

}

#endif
