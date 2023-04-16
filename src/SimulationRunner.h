//
// Created by stephane bourque on 2021-03-13.
//

#pragma once

#include <map>
#include <set>
#include <string>

#include <Poco/Thread.h>
#include <Poco/Environment.h>
#include "OWLSclient.h"
#include <RESTObjects/RESTAPI_OWLSobjects.h>

#include "CensusReport.h"
#include <libs/Scheduler.h>

namespace OpenWifi {

	class SimulationRunner {
	  public:
        explicit SimulationRunner(const OWLSObjects::SimulationDetails &Details, Poco::Logger &L, const std::string &id)
			: Details_(Details), Logger_(L), Id_(id)
            , Scheduler_(Poco::Environment::processorCount()*16){
        }

		void Stop();
		void Start();
        inline const OWLSObjects::SimulationDetails & Details() const { return Details_; }
        CensusReport & Report() { return CensusReport_; }

        void OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification> &pNf);
        void OnSocketError(const Poco::AutoPtr<Poco::Net::ErrorNotification> &pNf);
        void OnSocketShutdown(const Poco::AutoPtr<Poco::Net::ShutdownNotification> &pNf);

        const std::string & Id() const { return Id_; }

        inline void AddClientFd(std::int64_t fd, std::shared_ptr<OWLSclient> c) {
            std::lock_guard     G(Mutex_);
            Clients_fd_[fd] = c;
        }

        inline void RemoveClientFd(std::int64_t fd) {
            std::lock_guard     G(Mutex_);
            Clients_fd_.erase(fd);
        }

        void ProcessCommand(std::shared_ptr<OWLSclient> Client, nlohmann::json &Vars);
        Poco::Net::SocketReactor & Reactor() { return Reactor_; }

        inline auto & Scheduler() { return Scheduler_; }

	  private:
        my_mutex            Mutex_;
        OWLSObjects::SimulationDetails  Details_;
		Poco::Logger        &Logger_;
		Poco::Net::SocketReactor Reactor_;
		std::map<std::string, std::shared_ptr<OWLSclient>>      Clients_;
        std::map<std::int64_t, std::shared_ptr<OWLSclient>>     Clients_fd_;
		Poco::Thread        SocketReactorThread_;
		std::atomic_bool    Running_ = false;
		CensusReport        CensusReport_;
		std::string         State_{"stopped"};
        std::string         Id_;
        Bosma::Scheduler    Scheduler_;

        static void ProgressUpdate(SimulationRunner *s);

	};
} // namespace OpenWifi
