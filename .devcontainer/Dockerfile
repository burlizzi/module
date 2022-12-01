FROM airbus/leap
#FROM registry-1.docker.io/opensuse/amd64:42.3


ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Create the user
RUN zypper install -y make gcc kernel-default-devel tar sudo
RUN zypper install -y zsh git
RUN zypper install -y mc
RUN /home/vscode/.config/mc
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME

CMD /bin/sh 
USER vscode