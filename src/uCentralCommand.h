//
// Created by stephane bourque on 2021-04-03.
//

#ifndef UCENTRAL_CLNT_UCENTRALCOMMAND_H
#define UCENTRAL_CLNT_UCENTRALCOMMAND_H

#include <cstdint>
#include <string>

class uCentralCommand {
public:
private:
    uint64_t    ID_;
    std::string Method_;
};

/*
    "method" : "configure" ,
    "params" : {
        "serial" : <serial number> ,
        "uuid" : <waiting to apply this configuration>,
        "when" : Optional - <UTC time when to apply this config, 0 mean immediate, this is a suggestion>
        "config" : <JSON Document: New configuration”
     },
     "id" : <some number>
}

{    "jsonrpc" : "2.0" ,
     "result" : {
         "serial" : <serial number> ,
         "uuid" : <waiting to apply this configuration>,
         "status" : {
             "error" : 0 or an error number,
             "text" : <description of the error or success>
             "when" : <indication as to when this will be performed>,
             "rejected" : [
                            {   "parameter" : <JSON Document: text that caused the rejection> ,
                                "reason" : <why it was rejected>,
                                "substitution" : <JSON Document: replaced by this JSON. Optional>
                            }
                        ]
             }
         },
     "id" : <same number>
}
 */

class ConfigureCommand : public uCentralCommand {
public:
private:
};


#endif //UCENTRAL_CLNT_UCENTRALCOMMAND_H
