#!/usr/bin/bash

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Archlinux
requires=(
	ccache # Use ccache to speed up build
	meson  # Used for meson build
)

requires+=(
	autoconf-archive
	gcc
	git
	gobject-introspection
	gtk3
	intltool
	iso-codes
	make
	mate-common
	startup-notification
	which
)

infobegin "Update system"
pacman --noconfirm -Syu
infoend

infobegin "Install dependency packages"
pacman --noconfirm -S ${requires[@]}
infoend
