#!/usr/bin/env bash

DIST=$(cat /etc/os-release | grep -E "^ID=(.*)$" | cut -d "=" -f 2)

fatal() {
	echo -e "\033[1;31m[FATAL]: $1\033[0m"
	exit 1
}

error() {
	echo -e "\033[1;31m[ERROR]: $1\033[0m"
}

info() {
	echo -e "\033[1;32m$1\033[0m"
}

bold() {
	echo -e "\033[1m$1\033[0m"
}
