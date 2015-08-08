#!/bin/bash

CSDK_REVISION="0.9.1"

set -x

get_output_path() {
	scons -h | \
	awk \
		-v 'release=""' \
		-v 'os=""' \
		-v 'arch=""' \
		-v 'context=""' \
		'
		{
			if ( $1 == "RELEASE:" ) {
				context = "release";
			} else if ( $1 == "TARGET_OS:" ) {
				context = "os";
			} else if ( $1 == "TARGET_ARCH:" ) {
				context = "arch";
			}

			if ( $1 == "actual:" ) {
				if ( context == "release" ) {
					release = ( $2 == "True" ? "release" : "debug" );
					context = "";
				} else if ( context == "os" ) {
					os = $2;
					context = "";
				} else if ( context == "arch" ) {
					arch = $2;
					context = "";
				}
			}
		}
		END {
			print( "out/" os "/" arch "/" release );
		}
	'
}

install() {

	# Default prefix is /usr
	PREFIX="$(pwd)/../../deps/iotivity"

	# Install the first platform/arch/configuration we find
	PLATFORM="$(ls out | head -n 1)"

	if test "${PLATFORM}" = "darwin"; then
	  ARCH=x86_64
	else
	  ARCH="$(ls out/${PLATFORM} | head -n 1)"
	fi

	CONFIGURATION="$(ls out/${PLATFORM}/${ARCH} | grep -E "release|debug" | head -n 1)"

	LIBDIR="$( echo "${PREFIX}/lib" | sed 's@//*@/@g' )"
	INCLUDEDIR="$( echo "${PREFIX}/include" | sed 's@//*@/@g' )"

	OCTBSTACK_INCLUDEDIR=iotivity/resource/csdk/stack/include

	mkdir -p "${LIBDIR}" || return 1
	if test "${PLATFORM}" = "darwin"; then
	  cp out/${PLATFORM}/${ARCH}/${CONFIGURATION}/*.a ${LIBDIR} || return 1
	else
	  cp out/${PLATFORM}/${ARCH}/${CONFIGURATION}/liboctbstack.so ${LIBDIR} || return 1
	fi

	mkdir -p "${INCLUDEDIR}/iotivity/resource/csdk/stack" || return 1
	cp -a resource/csdk/stack/include "${INCLUDEDIR}/${OCTBSTACK_INCLUDEDIR}" || return 1
}

mkdir -p ./depbuild || exit 1

# Download and build iotivity from tarball
cd ./depbuild || exit 1
	wget -O iotivity.tar.gz 'https://gerrit.iotivity.org/gerrit/gitweb?p=iotivity.git;a=snapshot;h='"${CSDK_REVISION}"';sf=tgz' || exit 1
	tar xzf iotivity.tar.gz || exit 1

	# There should only be one directory inside this directory, so using the wildcard evaluates
	# exactly to it
	cd iotivity* || exit 1
		IOTIVITY_PATH="$( pwd )"
		OUTPUT_PATH="${IOTIVITY_PATH}/$( get_output_path )"
		test "x${OUTPUT_PATH}x" = "xx" && exit 1
		scons liboctbstack || { cat config.log; exit 1; }
		install || exit 1

cd ../../ || exit 1

rm -rf depbuild || exit 1
