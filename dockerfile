FROM alpine

RUN apk update && apk add -U make gcc-avr avr-libc git

RUN git clone https://github.com/G4TGJ/FreqGen5351.git -b tiny-1-series

COPY . .

WORKDIR .

