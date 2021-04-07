//
// Created by stephane bourque on 2021-04-07.
//

#include "SimStats.h"

SimStats * SimStats::instance_ = nullptr;

SimStats * Stats() { return SimStats::instance(); }
