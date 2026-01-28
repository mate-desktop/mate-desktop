#!/usr/bin/bash

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Debian
requires=(
	ccache # Use ccache to speed up build
	meson  # Used for meson build
)

# https://salsa.debian.org/debian-mate-team/mate-desktop
requires+=(
	autopoint
	git
	gobject-introspection
	gtk-doc-tools
	intltool
	iso-codes
	libdconf-dev
	libgirepository1.0-dev
	libglib2.0-dev
	libglib2.0-doc
	libgtk-3-dev
	libgtk-3-doc
	libstartup-notification0-dev
	libx11-dev
	libxml2-dev
	libxrandr-dev
	mate-common
)

infobegin "Update system"
apt-get update -qq
infoend

infobegin "Install dependency packages"
env DEBIAN_FRONTEND=noninteractive \
	apt-get install --assume-yes \
	${requires[@]}
infoend
