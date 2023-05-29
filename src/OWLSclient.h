//
// Created by stephane bourque on 2021-03-12.
//

#pragma once

#include <map>
#include <mutex>
#include <random>
#include <tuple>

#include <fmt/format.h>

#include <Poco/AutoPtr.h>
#include <Poco/JSON/Object.h>
#include <Poco/Logger.h>
#include <Poco/Net/SocketNotification.h>
#include <Poco/Net/SocketReactor.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Thread.h>

#include "framework/utils.h"

#include "OWLSdefinitions.h"
#include "MockElements.h"
#include "OWLSclientEvents.h"

namespace OpenWifi {

    class SimulationRunner;

	class OWLSclient {
	  public:
		OWLSclient(std::string SerialNumber,
                   Poco::Logger &Logger, SimulationRunner *runner, Poco::Net::SocketReactor &R);
        ~OWLSclient() {
            poco_debug(Logger_,fmt::format("{} simulator client done.", SerialNumber_));
        }

		bool Send(const std::string &Cmd);
		bool SendWSPing();
        bool SendObject(const Poco::JSON::Object::Ptr &O);
		void SetFirmware(const std::string &S = "sim-firmware-1") { Firmware_ = S; }

		[[nodiscard]] const std::string &Serial() const { return SerialNumber_; }
		[[nodiscard]] uint64_t UUID() const { return UUID_; }
		[[nodiscard]] uint64_t Active() const { return Active_; }
		[[nodiscard]] const std::string &Firmware() const { return Firmware_; }
		[[nodiscard]] bool Connected() const { return Connected_; }
		[[nodiscard]] inline uint64_t GetStartTime() const { return StartTime_; }

		static void DoConfigure(const std::shared_ptr<OWLSclient> &Client, uint64_t Id, Poco::JSON::Object::Ptr Params);
        static void DoReboot(const std::shared_ptr<OWLSclient> &Client, uint64_t Id, Poco::JSON::Object::Ptr Params);
        static void DoUpgrade(const std::shared_ptr<OWLSclient> &Client, uint64_t Id, Poco::JSON::Object::Ptr Params);
        static void DoFactory(const std::shared_ptr<OWLSclient> &Client, uint64_t Id, Poco::JSON::Object::Ptr Params);
        static void DoLEDs(const std::shared_ptr<OWLSclient> &Client, uint64_t Id, Poco::JSON::Object::Ptr Params);

        void UnSupportedCommand(std::lock_guard<std::mutex> &G,const std::shared_ptr<OWLSclient> &Client, uint64_t Id, const std::string &Method);

        using interface_location_t = std::tuple<ap_interface_types, std::string, radio_bands>;
        using associations_map_t = std::map<interface_location_t, MockAssociations>;

        void Disconnect(std::lock_guard<std::mutex> &Guard);

		void CreateAssociations(const interface_location_t &interface,const std::string &bssid, uint64_t min,
												   uint64_t max);
		void  CreateLanClients(uint64_t min, uint64_t max);

        Poco::JSON::Object::Ptr CreateStatePtr();
        Poco::JSON::Object::Ptr CreateLinkStatePtr();

		Poco::Logger &Logger() { return Logger_; };

		[[nodiscard]] uint64_t GetStateInterval() { return StatisticsInterval_; }
		[[nodiscard]] uint64_t GetHealthInterval() { return HealthInterval_; }

		void UpdateConfiguration();

		bool FindInterfaceRole(const std::string &role, ap_interface_types &interface);

		void Reset();

        inline std::uint64_t CountAssociations() {
            std::uint64_t   Total=0;
            for(const auto &[_,associations]:AllAssociations_) {
                Total += associations.size();
            }
            return Total;
        }

        inline const auto & Memory() { return Memory_; }
        inline const auto & Load() { return Load_; }

        void Update();
        [[nodiscard ]] inline auto Backoff() {
            if(Backoff_>300) {
                Backoff_ = 15;
            } else {
                Backoff_ *=2;
            }
            return Backoff_;
        }

        friend class SimulationRunner;

        friend void OWLSClientEvents::EstablishConnection(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
        friend void OWLSClientEvents::Reconnect(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
        // friend void OWLSClientEvents::Connect(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
        friend void OWLSClientEvents::Connect(std::lock_guard<std::mutex> & ClientGuard, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
        friend void OWLSClientEvents::Log(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner, std::uint64_t Severity, const std::string & LogLine);
        friend void OWLSClientEvents::State(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
        friend void OWLSClientEvents::HealthCheck(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
        friend void OWLSClientEvents::Update(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
        friend void OWLSClientEvents::WSPing(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
        friend void OWLSClientEvents::KeepAlive(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
        friend void OWLSClientEvents::Disconnect(std::lock_guard<std::mutex> &g, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner, const std::string &Reason, bool Reconnect);
        friend void OWLSClientEvents::CrashLog(std::lock_guard<std::mutex> &g, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
        friend void OWLSClientEvents::PendingConfig(std::lock_guard<std::mutex> &g, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);

    private:
		// std::recursive_mutex Mutex_;
        std::mutex              Mutex_;
		Poco::Logger            &Logger_;
		Poco::JSON::Object::Ptr CurrentConfig_;
		std::string             SerialNumber_;
		std::string             Firmware_;
		std::unique_ptr<Poco::Net::WebSocket> WS_;
        volatile bool           Valid_=false;
		std::uint64_t           Active_ = 0;
        std::uint64_t           UUID_ = 0;
		bool                    Connected_ = false;
		bool                    KeepRedirector_ = false;
		uint64_t                Version_ = 0;
		uint64_t                StartTime_ = Utils::Now();
		std::string             mac_lan;
		std::uint64_t           HealthInterval_ = 60;
		std::uint64_t           StatisticsInterval_ = 60;
		uint64_t                    bssid_index = 1;
        std::int64_t                fd_=-1;

        MockMemory                  Memory_;
        MockCPULoad                 Load_;

        std::uint16_t               Backoff_=0;

        SimulationRunner            *Runner_ = nullptr;
        Poco::Net::SocketReactor    &Reactor_;

		MockLanClients                              AllLanClients_;
		associations_map_t                          AllAssociations_;
		std::map<radio_bands, MockRadio>            AllRadios_;
		std::map<ap_interface_types, MockCounters>  AllCounters_;
		std::map<ap_interface_types, std::string>   AllInterfaceNames_;
		std::map<ap_interface_types, std::string>   AllInterfaceRoles_;
		std::map<ap_interface_types, std::string>   AllPortNames_;
	};
} // namespace OpenWifi
