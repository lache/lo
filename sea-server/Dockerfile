FROM ubuntu:18.04 as builder

RUN apt-get update && \
  apt-get upgrade -y && \
  apt-get install -y \
    gcc g++ build-essential automake autoconf cmake libboost-all-dev \
    tar gzip zip unzip wget curl

WORKDIR /opt
COPY . /opt

RUN make release

WORKDIR /opt/rtree
RUN find . -name "*.tgz" -exec tar xf {} \;

WORKDIR /opt/build-release
RUN ls .
RUN ls ../rtree
RUN ls ../assets
RUN ./sea-server --prepare

FROM ubuntu:18.04

COPY --from=builder /opt/build-release/sea-server /opt/bin/
COPY --from=builder /opt/assets /opt/assets
COPY --from=builder /opt/rtree/*.dat /opt/rtree/

WORKDIR /opt/bin

EXPOSE 3003/udp
CMD ["./sea-server"]

