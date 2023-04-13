//
// Created by stephane bourque on 2023-04-12.
//

#pragma once
#include "libs/Scheduler.h"
#include <framework/SubSystemServer.h>
#include <Poco/Environment.h>

namespace OpenWifi {

    class OWLSscheduler : public SubSystemServer {
    public:
        static auto instance() {
            static auto instance_ = new OWLSscheduler;
            return instance_;
        }

        int Start() final;
        void Stop() final;
        auto &Ref() { return Scheduler_; }

    private:
        Bosma::Scheduler   Scheduler_;
        OWLSscheduler() noexcept
                : SubSystemServer("OWLSScheduler", "SIM-SCHEDULER", "scgeduler"),
                  Scheduler_(Poco::Environment::processorCount()*16){
        }

    };

    inline auto OWLSscheduler() {
        return OWLSscheduler::instance();
    }

} // OpenWifi
