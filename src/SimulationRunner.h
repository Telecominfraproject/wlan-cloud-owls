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
#include <RESTObjects/RESTAPI_SecurityObjects.h>

namespace OpenWifi {

	class SimulationRunner {
	  public:
        explicit SimulationRunner(const OWLSObjects::SimulationDetails &Details, Poco::Logger &L, const std::string &id, const SecurityObjects::UserInfo &uinfo)
			: Details_(Details), Logger_(L), Id_(id)
            , Scheduler_(Poco::Environment::processorCount()*16)
            , UInfo_(uinfo){
        }

		void Stop();
		void Start();
        inline const OWLSObjects::SimulationDetails & Details() const { return Details_; }
        CensusReport & Report() { return CensusReport_; }

        void OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification> &pNf);
        void OnSocketError(const Poco::AutoPtr<Poco::Net::ErrorNotification> &pNf);
        void OnSocketShutdown(const Poco::AutoPtr<Poco::Net::ShutdownNotification> &pNf);

        const std::string & Id() const { return Id_; }

        inline void AddClientFd(std::int64_t fd, const std::shared_ptr<OWLSclient> &c) {
            std::lock_guard     G(SocketFdMutex_);
            Clients_fd_[fd] = c;
        }

        inline void RemoveClientFd(std::int64_t fd) {
            std::lock_guard     G(SocketFdMutex_);
            Clients_fd_.erase(fd);
        }

        void ProcessCommand(std::lock_guard<std::mutex> &G, const std::shared_ptr<OWLSclient> &Client, Poco::JSON::Object::Ptr Vars);
        // Poco::Net::SocketReactor & Reactor() { return Reactor_; }

        inline auto & Scheduler() { return Scheduler_; }
        inline bool Running() { return Running_; }

	  private:
        std::mutex          SocketFdMutex_;
        my_mutex            Mutex_;
        OWLSObjects::SimulationDetails  Details_;
		Poco::Logger        &Logger_;
		std::vector<std::unique_ptr<Poco::Net::SocketReactor>>   SocketReactorPool_;
        std::vector<std::unique_ptr<Poco::Thread>>               SocketReactorThreadPool_;
		std::map<std::string, std::shared_ptr<OWLSclient>>      Clients_;
        std::map<std::int64_t, std::shared_ptr<OWLSclient>>     Clients_fd_;
		std::atomic_bool    Running_ = false;
		CensusReport        CensusReport_;
		std::string         State_{"stopped"};
        std::string         Id_;
        Bosma::Scheduler    Scheduler_;
        SecurityObjects::UserInfo   UInfo_;
        std::uint64_t       NumberOfReactors_=0;

        static void ProgressUpdate(SimulationRunner *s);

	};
} // namespace OpenWifi
