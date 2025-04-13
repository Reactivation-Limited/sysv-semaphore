#!/bin/bash -e

npm ci
npm run swig

for ARCH in arm64 i386 amd64
do
  npx prebuildify --strip --arch $ARCH
done

# TMP dir must not be inside a bind mount
TMP=/tmp npm run test
