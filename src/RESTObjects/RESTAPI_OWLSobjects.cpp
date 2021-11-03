//
// Created by stephane bourque on 2021-08-31.
//

#include "RESTAPI_OWLSobjects.h"

namespace OpenWifi::OWLSObjects {

    void Dashboard::to_json(Poco::JSON::Object &Obj) const {

    }

    bool Dashboard::from_json(const Poco::JSON::Object::Ptr &Obj) {
        return true;
    }

    void Dashboard::reset() {

    }
}
