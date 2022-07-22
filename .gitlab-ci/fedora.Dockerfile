FROM fedora:39

RUN dnf -y update \
 && dnf -y install \
    clang \
    clang-analyzer \
    gcc \
    gcovr \
    gi-docgen \
    meson \
    json-glib-devel \
    rest-devel \
    libsoup3-devel \
    gnome-online-accounts-devel \
    gobject-introspection-devel \
 && dnf clean all


# Enable sudo for wheel users
RUN sed -i -e 's/# %wheel/%wheel/' -e '0,/%wheel/{s/%wheel/# %wheel/}' /etc/sudoers

ARG HOST_USER_ID=5555
ENV HOST_USER_ID ${HOST_USER_ID}
RUN useradd -u $HOST_USER_ID -G wheel -ms /bin/bash user

USER user
WORKDIR /home/user

ENV LANG C.UTF-8