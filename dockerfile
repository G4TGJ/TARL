FROM opthomasprime/avr-gcc

RUN mkdir /home/dev
WORKDIR /home/dev
RUN git clone https://github.com/G4TGJ/FreqGen5351.git
RUN git clone https://github.com/G4TGJ/MorseKeyer.git

COPY . TARL

WORKDIR .

