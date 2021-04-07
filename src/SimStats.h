//
// Created by stephane bourque on 2021-04-07.
//

#ifndef UCENTRALSIM_SIMSTATS_H
#define UCENTRALSIM_SIMSTATS_H

#include <mutex>

struct StatsReport {
    uint64_t Connected;
    uint64_t Total;
    uint64_t TX;
    uint64_t RX;
    uint64_t InMsgs;
    uint64_t OutMsgs;
};

class SimStats {
    typedef std::recursive_mutex        my_mutex;
    typedef std::lock_guard<my_mutex>   my_guard;

public:
    void Connect() {
        my_guard Lock(Mutex_);
        Connected_++;
    }
    void Disconnect() {
        my_guard Lock(Mutex_);
        Connected_--;
    }
    void AddClients(uint64_t C) {
        my_guard Lock(Mutex_);
        NumDevices_ += C;
    }

    static SimStats * instance() {
        if(instance_ == nullptr)
            instance_ = new SimStats;
        return instance_;
    }

    void AddRX(uint64_t N) {
        my_guard Lock(Mutex_);
        RX_ += N;
    }

    void AddOutMsg() {
        my_guard Lock(Mutex_);
        OutMsgs_++;
    }

    void AddInMsg() {
        my_guard Lock(Mutex_);
        InMsgs_++;
    }

    void AddTX(uint64_t N) {
        my_guard Lock(Mutex_);
        TX_ += N;
    }

    void Report( StatsReport & Report ) {
        my_guard Lock(Mutex_);

        Report.Connected = Connected_;
        Report.Total = NumDevices_;
        Report.RX = RX_;
        Report.TX = TX_;
        Report.InMsgs = InMsgs_;
        Report.OutMsgs = OutMsgs_;
    }

private:
    static SimStats     * instance_;
    my_mutex            Mutex_;
    uint64_t            NumDevices_ = 0 ;
    uint64_t            Connected_ = 0 ;
    uint64_t            RX_ = 0;
    uint64_t            TX_ = 0;
    uint64_t            InMsgs_ = 0 ;
    uint64_t            OutMsgs_ = 0;
};

SimStats * Stats();

#endif //UCENTRALSIM_SIMSTATS_H
