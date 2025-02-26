# build for linux

FROM debian
RUN mkdir /root/project
VOLUME /root/project

RUN apt-get update
RUN apt-get install -y build-essential npm wget

WORKDIR /root

# build swig 4.2 from source as debian bookworm has only 4.1
# required for -napi swig target
RUN git clone https://salsa.debian.org/debian/swig.git --depth 1
# fetch swig build deps
RUN apt-get install -y automake libtool bison libpcre2-dev

WORKDIR /root/swig
RUN ./autogen.sh
RUN ./configure
RUN make
RUN make install

WORKDIR /root/project

RUN npm cache clean -f
RUN npm install -g n
RUN n stable

CMD . scripts/prebuild-linux.sh
