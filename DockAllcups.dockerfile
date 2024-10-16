FROM ubuntu:noble
RUN export DEBIAN_FRONTEND=noninteractive && \
    TZ=Etc/UTC && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    tzdata unzip jq python3.7 clang make
RUN ln /usr/bin/clang /usr/bin/gcc
RUN mkdir /sysprog

WORKDIR /sysprog
RUN mkdir ./solution
RUN mkdir ./results

ENV RESOURCES_DIR_MOUNT='/tmp/data'
ENV SOLUTION_MOUNT='/sysprog/solution'
ENV RESULT_LOCATION='/sysprog/results/output.json'

CMD ["/tmp/data/test.sh"]