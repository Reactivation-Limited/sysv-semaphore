# build for linux

FROM debian
RUN mkdir /root/project
VOLUME /root/project

RUN apt-get update
RUN apt-get install -y build-essential npm wget cmake

WORKDIR /root/project

RUN npm cache clean -f
RUN npm install -g n
RUN n stable

CMD . scripts/prebuild-linux.sh