#!/bin/bash

PACKCHK_VERSION=1.3.95
SVDCONV_VERSION=3.3.42

REPO_URL=https://github.com/Open-CMSIS-Pack/devtools
DOWNLOAD_URL=${REPO_URL}/releases/download/
DIRNAME=$(dirname $0)

set -o pipefail

function usage {
  echo "$(basename $0) [-h|--help] [-f|--force]"
  echo ""
  echo "Arguments:"
  echo "  -h|--help   Print this usage message and exit."
  echo "  -f|--force  Force (re)download."
  echo ""
  echo "Environment:"
  echo " curl"
  echo " sha256sum"
  echo ""
}

function fetch {
  mkdir -p ${DIRNAME}/$2
  pushd ${DIRNAME}/$2 >/dev/null
  curl -O -L $1
  unzip -o $(basename $1)
  rm $(basename $1)
  popd >/dev/null
}

POSITIONAL=()
while [[ $# -gt 0 ]]
do
  key="$1"

  case $key in
    '-h'|'--help')
      usage
      exit 1
    ;;
    '-f'|'--force')
      FORCE=1
    ;;
    *)    # unknown option
      POSITIONAL+=("$1") # save it in an array for later
    ;;
  esac
  shift # past argument
done
set -- "${POSITIONAL[@]}" # restore positional parameters

fetch "${DOWNLOAD_URL}/tools%2Fpackchk%2F${PACKCHK_VERSION}/packchk-${PACKCHK_VERSION}-darwin64.zip" Darwin64
fetch "${DOWNLOAD_URL}/tools%2Fpackchk%2F${PACKCHK_VERSION}/packchk-${PACKCHK_VERSION}-linux64.zip" Linux64
fetch "${DOWNLOAD_URL}/tools%2Fpackchk%2F${PACKCHK_VERSION}/packchk-${PACKCHK_VERSION}-windows64.zip" Win32

fetch "${DOWNLOAD_URL}/tools%2Fsvdconv%2F${SVDCONV_VERSION}/svdconv-${SVDCONV_VERSION}-darwin64.zip" Darwin64
fetch "${DOWNLOAD_URL}/tools%2Fsvdconv%2F${SVDCONV_VERSION}/svdconv-${SVDCONV_VERSION}-linux64.zip" Linux64
fetch "${DOWNLOAD_URL}/tools%2Fsvdconv%2F${SVDCONV_VERSION}/svdconv-${SVDCONV_VERSION}-windows64.zip" Win32

exit 0
