#!/usr/bin/env bash

. $(dirname $0)/shellenv.sh

set -e
declare -A TOOLCHAINS

make_labs_env() {
	case "${DIST}" in
	"ubuntu" | "debian")
		if command -v gdb-multiarch &>/dev/null; then
			TOOLCHAINS["gdb"]="gdb-multiarch"
		else
			TOOLCHAINS["gdb"]="gdb"
		fi
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
		error "Unsupported Linux Distribution: ${DIST}"
		error "Supported OS Distributions are: ubuntu/debian, fedora, arch, gentoo, opensuse"
		fatal "If you want to add support for your distribution, please submit a PR."
		;;
	esac
}

def_labs_env() {
	rm -f $(dirname $0)/env_generated.mk
	touch $(dirname $0)/env_generated.mk
	for toolchain in "${!TOOLCHAINS[@]}"; do
		LHS=$(echo $toolchain | tr '[:lower:]' '[:upper:]')
		RHS=${TOOLCHAINS[$toolchain]}
		echo "${LHS} := ${RHS}" >>$(dirname $0)/env_generated.mk
	done
}

make_labs_env
def_labs_env
