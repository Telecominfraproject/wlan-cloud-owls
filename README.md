# OWLS for TIP 2.0
This the OpenWifi Load Simulator for TIP 2.x. OWLS is a tool to help test platform sizing for large TIP 2.x deployments. The goal is to flood a TIP gateway with thousands of simulated Access Points. The simulation performs full mTLS, connection, state, health checks, log messages over the whole period. Exponential back-off is included when reconnections are need to the gateway. The simulated also response to some commands: blink, factory reset, upgrade, reboot.

## Before running a simulation
### Get a simulator key
You must create a digicert certs for your simulator(this is using the same method used to create AP certs). The serial number for the simulator must start with the following digits: 53494d and will be a total of 12 hex-digits. This is your simulator ID. You will receive 1 archive file containing 4 files. You must rename cert.pem to device-cert.pem, and key.pem to device-key.pem. Copy both files in the `certs` directory of OWLS. You can safely delete the other 2 files.

### Prepare your OpenWifi Gateway
You must be running a gateway version 2.4 or later. In your properties file, you should enter the following: 

```
simulatorid = <you simulatorid>
```
as an example
```
simulatorid = 53494d010101
```

Make sure you restart your gateway afterwards.

## The OWLS UI
Using the OWLS UI, you cana create simulation definitions, and then run a live simulation based on the definition. You will need to know the public address for your gateway. It should look like https://openwifi.myispname.com:15002.

