//
// Created by stephane bourque on 2021-04-03.
//

#ifndef UCENTRAL_CLNT_OWLSEVENT_H
#define UCENTRAL_CLNT_OWLSEVENT_H

#include "Poco/JSON/Object.h"
#include "OWLSclient.h"

#include "OWLSclient.h"
#include "OWLSdefinitions.h"

namespace OpenWifi {
	class OWLSevent {
	  public:
		explicit OWLSevent(std::shared_ptr<OWLSclient> Client)
			: Client_(std::move(Client)) {}
		virtual bool Send() = 0;

	  protected:
		bool SendObject(Poco::JSON::Object &Obj);
		std::shared_ptr<OWLSclient> Client_;
	};

	class OWLSConnectEvent : public OWLSevent {
	  public:
		explicit OWLSConnectEvent(std::shared_ptr<OWLSclient> Client)
			: OWLSevent(std::move(Client)){};
		bool Send() override;

	  private:
	};

	class OWLSStateEvent : public OWLSevent {
	  public:
		explicit OWLSStateEvent(std::shared_ptr<OWLSclient> Client)
			: OWLSevent(std::move(Client)){};
		bool Send() override;

	  private:
	};

	class OWLSHealthCheckEvent : public OWLSevent {
	  public:
		explicit OWLSHealthCheckEvent(std::shared_ptr<OWLSclient> Client)
			: OWLSevent(std::move(Client)){};
		bool Send() override;

	  private:
	};

	class OWLSLogEvent : public OWLSevent {
	  public:
		explicit OWLSLogEvent(std::shared_ptr<OWLSclient> Client, std::string LogLine,
                          uint64_t Severity)
			: OWLSevent(std::move(Client)), LogLine_(std::move(LogLine)), Severity_(Severity){};
		bool Send() override;

	  private:
		std::string LogLine_;
		uint64_t Severity_;
	};

	class OWLSCrashLogEvent : public OWLSevent {
	  public:
		explicit OWLSCrashLogEvent(std::shared_ptr<OWLSclient> Client)
			: OWLSevent(std::move(Client)){};
		bool Send() override;

	  private:
	};

	class OWLSConfigChangePendingEvent : public OWLSevent {
	  public:
		explicit OWLSConfigChangePendingEvent(std::shared_ptr<OWLSclient> Client)
			: OWLSevent(std::move(Client)){};
		bool Send() override;

	  private:
	};

	class OWLSKeepAliveEvent : public OWLSevent {
	  public:
		explicit OWLSKeepAliveEvent(std::shared_ptr<OWLSclient> Client)
			: OWLSevent(std::move(Client)){};
		bool Send() override;

	  private:
	};

	class OWLSRebootEvent : public OWLSevent {
	  public:
		explicit OWLSRebootEvent(std::shared_ptr<OWLSclient> Client)
			: OWLSevent(std::move(Client)){};
		bool Send() override;

	  private:
	};

	class OWLSDisconnectEvent : public OWLSevent {
	  public:
		explicit OWLSDisconnectEvent(std::shared_ptr<OWLSclient> Client)
			: OWLSevent(std::move(Client)){};
		bool Send() override;

	  private:
	};

	class OWLSWSPingEvent : public OWLSevent {
	  public:
		explicit OWLSWSPingEvent(std::shared_ptr<OWLSclient> Client)
			: OWLSevent(std::move(Client)){};
		bool Send() override;

	  private:
	};

    class OWLSUpdate : public OWLSevent {
    public:
        explicit OWLSUpdate(std::shared_ptr<OWLSclient> Client)
                : OWLSevent(std::move(Client)){};
        bool Send() override;

    private:
    };

} // namespace OpenWifi

#endif // UCENTRAL_CLNT_OWLSEVENT_H
