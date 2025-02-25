#!/bin/bash -e

npm ci
npm test
npm run swig
for ARCH in arm64 i386 amd64
do
  npx prebuildify --strip --arch $ARCH
done
