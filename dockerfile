FROM alpine

RUN apk update && apk add -U make gcc-avr avr-libc git

RUN mkdir /home/dev
WORKDIR /home/dev
RUN git clone https://github.com/G4TGJ/FreqGen5351.git

COPY . TARL

WORKDIR .

