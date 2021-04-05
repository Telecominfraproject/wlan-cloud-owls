# ucentralsim
uCentral simulator to test [uCentralGW](https://github.com/stephb9959/ucentralsim) scalability. You may run this 
application against your own gateway to see how to set certain settings, test memory requirements, and CPU sizing.

## Build it or Docker it
Red pill or Blue pill... Well, if you care to build this, you must follow the same instructions as you would when
building [uCentralGW](https://github.com/stephb9959/ucentralsim). The only difference is that you will use this repository
instead of the uCentralGW repository. This build have the same requirements with the different platforms and Poco. if all 
you want is to play, go easy on yourself and use the Docker version.

## Docker
Choose the directory where you will run your docker instance, and let's call the the `root`. Here is what you need to do
when is the root:

```shell
mkdir certs
mkdir logs
```

After you have your certificate and your key, you need to copy them in the `certs` directory under the name `client-cert.key` and 
`client-key.pem`. After all this is done, you hsould have the following

```shell

root --+
       +--- certs
       |      |
       |      +--- client-key.pem
       |      +--- client-cert.pem
       |
       +--- logs
       |
       +--- ucentralsim.properties
```

## Certificates
If you used the uCentralGW, follow its certificates [generation instructions](https://github.com/stephb9959/ucentralgw/blob/main/README.md/#certificates) and use one of the `dev-X-cert.pem` file and opy it in your 
`certs` directory.

## The configuration file: `ucentralsim.properties`
You can find the base file [here](https://github.com/stephb9959/ucentralsim/blob/main/ucentralsim.properties). If you have followed the instructions above,
the only entries that need changing are the following:

```asm
ucentral.simulation.uri = wss://localhost:15002
ucentral.simulation.maxclients = 2000
ucentral.simulation.serialbase = 22334400
```

### `ucentral.simulation.uri`
This should be the URI of your uCentralGW's websocket interface. This would be the same host or IP address as you have set on your devices. Do not forget 
to use the proper port which defaults to 15002.

### `ucentral.simulation.maxclients`
How many simulated clients would you like to have? Have fun with this one. Please notice that the more clients you add, you may need to increase 
some sockets limits on your gateway or the host where you are running this application. 1 socket per client is needed. Bare that in mind.

### `ucentral.simulation.serialbase`
All the serial numbers generated for these devices will begin with  this base. This is a hex string and should be 10 characters in order to create a 
real simulation bed. 

## Running the docker simulation
Simply run the `docker_run.sh` script in order to start the simulation. To stop the simulation,

```shell
docker stop ucentralsim
```

## Verify the simulation is running
To verify that the simulation is running, simply go into your `logs` directory and type

```shell
tail -f sample.log
```

This will show you what the simulator is doing.
