//
// Created by stephane bourque on 2021-03-13.
//

#pragma once

#include <map>
#include <set>
#include <string>

#include "Poco/Thread.h"
#include "uCentralClient.h"

namespace OpenWifi {
	class Simulator : public Poco::Runnable {
	  public:
		Simulator(uint64_t Index, std::string SerialStart, uint64_t NumClients, Poco::Logger &L)
			: Logger_(L), Index_(Index), SerialStart_(std::move(SerialStart)),
			  NumClients_(NumClients) {}

		void run() override;
		void stop();
		void Initialize(/* Poco::Logger & ClientLogger*/);

		void Cancel() {
			State_ = "cancel";
			SocketReactorThread_.wakeUp();
		}

	  private:
		Poco::Logger &Logger_;
		my_mutex Mutex_;
		Poco::Net::SocketReactor Reactor_;
		std::map<std::string, std::shared_ptr<uCentralClient>> Clients_;
		Poco::Thread SocketReactorThread_;
		std::atomic_bool Running_ = false;
		uint64_t Index_ = 0;
		std::string SerialStart_;
		uint64_t NumClients_ = 0;
		CensusReport CensusReport_;
		std::string State_{"stopped"};
	};
} // namespace OpenWifi
