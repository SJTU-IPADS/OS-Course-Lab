#!/bin/bash

. $(dirname $0)/env_setup.sh

declare -A TOOLCHAINS

install_toolchains(){
	case "${DIST}" in
		"ubuntu")
			TOOLCHAINS["gdb"]="gdb-multiarch"
			;;
		"fedora")
			TOOLCHAINS["gdb"]="gdb"
			;;
		"arch")
			TOOLCHAINS["gdb"]="gdb"
			;;
		"gentoo")
			TOOLCHAINS["gdb"]="gdb"
			;;
		"*suse*")
			TOOLCHAINS["gdb"]="gdb"
			;;
		*)
			error "Unsupported Linux Distribution: ${OS}"
			error "Supported OS Distributions are: ubuntu, fedora, arch, gentoo, opensuse"
			fatal "If you want to add support for your distribution, please submit a PR."
			;;
	esac
}

def_labs_env(){
	rm -f $(dirname $0)/env_generated.mk
	for toolchain in "${TOOLCHAINS[@]}"; do
		LHS=$(echo $toolchain | tr '[:lower:]' '[:upper:]')
		RHS=${toolchain}
		echo "${LHS}:=${RHS}" >> $(dirname $0)/env_generated.mk
	done
}

install_toolchains
def_labs_env
