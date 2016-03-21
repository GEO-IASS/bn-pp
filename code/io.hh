#ifndef _BN_IO_FILE_H_
#define _BN_IO_FILE_H_

#include <unordered_map>

#include "model.hh"

namespace bn {

int
read_uai_model(const char *filename, BN **model);

int
read_uai_model(const char *filename, MN **model);

int
read_uai_evidence(const char *filename, std::unordered_map<unsigned,unsigned> &evidence);

}

#endif
