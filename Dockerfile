FROM ubuntu:18.04
WORKDIR /code
RUN apt-get update;\
    apt-get install -y --install-recommends gcc-multilib make
CMD ["make", "all"]
