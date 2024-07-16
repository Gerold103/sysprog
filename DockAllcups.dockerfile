FROM ubuntu:noble
RUN apt update
RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt -y install tzdata
RUN apt install -y clang make
RUN ln /usr/bin/clang /usr/bin/gcc
RUN mkdir /sysprog

WORKDIR /sysprog
RUN mkdir ./solution
RUN mkdir ./results

ENV RESOURCES_DIR_MOUNT='/tmp/data'
ENV SOLUTION_MOUNT='/sysprog/solution'
ENV RESULT_LOCATION='/sysprog/results/output.json'

RUN mkdir /sysprog/3
ADD 3/test.c /sysprog/3/

RUN mkdir /sysprog/4
ADD 4/test.c /sysprog/4/

ADD utils /sysprog/utils
ADD test.sh /sysprog/test.sh

# CMD ["python3", "main.py"]
