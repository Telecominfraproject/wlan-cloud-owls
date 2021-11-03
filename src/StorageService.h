//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#ifndef UCENTRAL_USTORAGESERVICE_H
#define UCENTRAL_USTORAGESERVICE_H

#include "framework/MicroService.h"
#include "framework/StorageClass.h"
#include "storage/storage_simulations.h"
#include "storage/storage_results.h"

namespace OpenWifi {

    class Storage : public StorageClass {
        public:
            static Storage *instance() {
                if (instance_ == nullptr) {
                    instance_ = new Storage;
                }
                return instance_;
            }

            OpenWifi::SimulationDB & SimulationDB() { return *SimulationDB_; }
            OpenWifi::SimulationResultsDB & SimulationResultsDB() { return *SimulationResultsDB_; }

            int Start() override;
            void Stop() override;

          private:
            static Storage      							*instance_;
            std::unique_ptr<OpenWifi::SimulationDB>         SimulationDB_;
            std::unique_ptr<OpenWifi::SimulationResultsDB>  SimulationResultsDB_;
    };

    inline class Storage * StorageService() { return Storage::instance(); }

}  // namespace

#endif //UCENTRAL_USTORAGESERVICE_H
