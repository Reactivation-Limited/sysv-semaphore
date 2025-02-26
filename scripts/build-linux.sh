#!/bin/bash -e

docker build --tag OSiX/linux .
docker run --interactive --volume .:/root/project OSiX/linux
