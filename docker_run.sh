#!/bin/sh

HUBNAME=tip-tip-wlan-cloud-ucentral.jfrog.io
IMAGE_NAME=ucentralsim
DOCKER_NAME=$HUBNAME/$IMAGE_NAME

CONTAINER_NAME=ucentralsim

#stop previously running images
docker container stop $CONTAINER_NAME
docker container rm $CONTAINER_NAME --force

if [[ ! -d logs ]]
then
    mkdir logs
fi

if [[ ! -d certs ]]
then
  echo "certs directory does not exist. Please create and add the proper certificates."
  exit 1
fi

if [[ ! -f ucentralsim.properties ]]
then
  echo "Configuration file ucentral.properties is missing in the current directory"
  exit 2
fi

docker run -d --init \
              --volume="$PWD:/ucentralsim-data" \
              -e UCENTRAL_CLIENT_ROOT="/ucentralsim-data" \
              -e UCENTRAL_CLIENT_CONFIG="/ucentralsim-data" \
              --name="$CONTAINER_NAME" $DOCKER_NAME

