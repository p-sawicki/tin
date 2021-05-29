FROM ubuntu:latest
RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata
RUN apt-get update && apt-get install -y pkg-config git libssl-dev cmake build-essential python3-pip
RUN pip3 install argparse ipcqueue