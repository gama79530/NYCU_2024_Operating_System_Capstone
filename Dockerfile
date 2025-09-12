FROM ubuntu:22.04

ARG USERNAME=test
ARG USER_UID=1000
ARG USER_GID=1000

# Prevent interactive operations
ENV DEBIAN_FRONTEND=noninteractive

USER root

RUN apt-get update \
&&  apt-get install -y locales \
&&  apt-get install -y python3 python3-setuptools python3-pip python3-distutils python3-venv \
&&  rm -rf /var/lib/apt/lists/* 

RUN pip3 install --no-cache-dir pyserial

# Set locale settings
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8 LANGUAGE=en_US:en LC_ALL=en_US.UTF-8

RUN printf '%s\n' \
  'if [ -f /usr/share/bash-completion/completions/git ]; then' \
  '  . /usr/share/bash-completion/completions/git' \
  'fi' >> ~/.bashrc