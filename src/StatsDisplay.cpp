//
// Created by stephane bourque on 2021-04-07.
//

#include "StatsDisplay.h"
#include "SimStats.h"

void StatsDisplay::run() {

    char Buffer[256];
    StatsReport R;

    while(!Stop_)
    {
        Poco::Thread::sleep(3000);

        Stats()->Report(R);

        snprintf(Buffer,sizeof(Buffer),"Connected:%06llu  TX:%09llu  RX:%09llu  IN:%08llu OUT:%08llu",R.Connected,R.TX,R.RX,R.InMsgs,R.OutMsgs);
        std::cout << Buffer << std::endl;
    }
}