FROM alpine

RUN apk add --update --no-cache \
    cmake \
    make \
    g++ \
    git \
  && rm -rf /var/cache/apk/*

ADD ./ /MMDVMHost
WORKDIR /MMDVMHost
RUN make \
&& cp MMDVMHost /usr/local/bin

RUN mkdir /var/log/mmdvmhost

VOLUME /MMDVMHost
VOLUME /var/log/mmdvmhost

WORKDIR /MMDVMHost

CMD ["MMDVMHost", "/MMDVMHost/MMDVM.ini"]

