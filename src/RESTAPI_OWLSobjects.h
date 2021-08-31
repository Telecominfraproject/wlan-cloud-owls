//
// Created by stephane bourque on 2021-08-31.
//

#ifndef UCENTRALSIM_RESTAPI_OWLSOBJECTS_H
#define UCENTRALSIM_RESTAPI_OWLSOBJECTS_H

#include "Poco/JSON/Object.h"

namespace OpenWifi::OWLSObjects {

    struct Dashboard {
        int O;

        void to_json(Poco::JSON::Object &Obj) const;
        bool from_json(const Poco::JSON::Object::Ptr &Obj);
        void reset();

    };

}


#endif //UCENTRALSIM_RESTAPI_OWLSOBJECTS_H
