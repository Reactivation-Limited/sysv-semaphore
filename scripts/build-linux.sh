#!/bin/bash -ex

docker build --tag sysv-semaphore-build.linux .
docker run --rm --name sysv-semaphore-build.linux --interactive --volume .:/root/project sysv-semaphore-build.linux
