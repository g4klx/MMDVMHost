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

VOLUME /MMDVMHost
WORKDIR /MMDVMHost

CMD ["MMDVMHost", "/MMDVMHost/MMDVM.ini"]

