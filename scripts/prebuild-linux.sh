#!/bin/bash -ex

npm ci

npm run gtest

for ARCH in arm64 i386 amd64
do
  npx prebuildify --strip --arch $ARCH
done

# TMP dir must not be inside a bind mount
TMP=/tmp npm run test
