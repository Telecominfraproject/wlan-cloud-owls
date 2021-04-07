//
// Created by stephane bourque on 2021-04-07.
//
#include <ostream>
#include <iostream>
#include <iomanip>

#include "StatsDisplay.h"
#include "SimStats.h"

void StatsDisplay::run() {

    StatsReport R{0};

    while(!Stop_)
    {
        Poco::Thread::sleep(3000);
        Stats()->Report(R);
        std::cout << "Connected:" << std::setw(6) << R.Connected << " TX:" << std::setw(10) << R.TX << " RX:" << std::setw(10) << R.RX <<
        " IN:" << std::setw(9) << R.InMsgs << " OUT:" << std::setw(9) << R.OutMsgs << std::endl;
    }
}