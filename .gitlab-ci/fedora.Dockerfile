FROM fedora:41

RUN dnf -y update \
 && dnf -y install \
    clang \
    clang-analyzer \
    gcc \
    gcovr \
    gi-docgen \
    meson \
    json-glib-devel \
    libsoup3-devel \
    gnome-online-accounts-devel \
    gobject-introspection-devel \
    git \
 && dnf clean all

 # Install recent uhttpmock
 RUN git clone https://gitlab.freedesktop.org/pwithnall/uhttpmock/ && cd uhttpmock && meson setup build -Dprefix=/usr -Dintrospection=false -Dvapi=disabled -Dgtk_doc=false && ninja -C build && ninja -C build install && cd .. && rm -rf uhttpmock


# Enable sudo for wheel users
RUN sed -i -e 's/# %wheel/%wheel/' -e '0,/%wheel/{s/%wheel/# %wheel/}' /etc/sudoers

ARG HOST_USER_ID=5555
ENV HOST_USER_ID ${HOST_USER_ID}
RUN useradd -u $HOST_USER_ID -G wheel -ms /bin/bash user

USER user
WORKDIR /home/user

ENV LANG C.UTF-8