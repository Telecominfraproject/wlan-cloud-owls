//
// Created by stephane bourque on 2021-04-07.
//

#ifndef UCENTRALSIM_STATSDISPLAY_H
#define UCENTRALSIM_STATSDISPLAY_H

#include "Poco/Thread.h"

class StatsDisplay : public Poco::Runnable {
public:
    void run() override;
    void Stop() { Stop_ = true ; }
private:
    volatile bool Stop_ = false;
};


#endif //UCENTRALSIM_STATSDISPLAY_H
