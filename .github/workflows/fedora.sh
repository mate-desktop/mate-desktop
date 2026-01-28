#!/usr/bin/bash

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Fedora
requires=(
	ccache # Use ccache to speed up build
	meson  # Used for meson build
)

requires+=(
	autoconf-archive
	cairo-gobject-devel
	dconf-devel
	desktop-file-utils
	gcc
	git
	gobject-introspection-devel
	gtk3-devel
	iso-codes-devel
	itstool
	make
	mate-common
	redhat-rpm-config
	startup-notification-devel
)

infobegin "Update system"
dnf update -y
infoend

infobegin "Install dependency packages"
dnf install -y ${requires[@]}
infoend
