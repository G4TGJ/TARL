FROM ubuntu:18.04

RUN apt-get update && apt-get install -y \
    git \
    gcc-avr \
    avr-libc \
    make

RUN mkdir /home/dev
WORKDIR /home/dev

RUN git clone https://github.com/G4TGJ/FreqGen5351.git
RUN git clone https://github.com/G4TGJ/MorseKeyer.git
RUN git clone https://github.com/G4TGJ/MSFClock.git

COPY . TARL

WORKDIR .

