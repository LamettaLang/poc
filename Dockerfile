FROM gcc:15.2-trixie

# install python
RUN apt update
RUN apt install -y python3 python3-pip python3-venv sudo build-essential gdb

# go to the project
WORKDIR /usr/mybcp

# set up user, so generated files are owned properly.
# we dont really care about the home directory, it just has to match the compose file
ARG REMOTE_UID
ARG REMOTE_GID
ARG REMOTE_USER
ENV DEBIAN_FRONTEND=noninteractive

# if we run in a dev container, don't bother setting up a user
RUN addgroup --gid "${REMOTE_GID?:REMOTE_GID not set}" "${REMOTE_USER?:REMOTE_USER not set}" && \
  adduser --gid "${REMOTE_GID}" --uid "${REMOTE_UID?:REMOTE_UID not set}" --home /home/mybcp "${REMOTE_USER}" && chown -R ${REMOTE_USER?:}:${REMOTE_USER?:} /home/mybcp && \
  adduser "${REMOTE_USER}" sudo && \
  echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# system build dependencies
# RUN apt install -y <packages>

# activate user (will silently fail if not set aka running as dev container)
USER ${REMOTE_USER}

ENTRYPOINT ["./entrypoint.sh"]
