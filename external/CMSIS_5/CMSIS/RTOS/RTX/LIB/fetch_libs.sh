#!/bin/bash

VERSION=4.82.0
if [ -z "$JENKINS_FAMILY_ENV" ]; then
    ARTIFACTORY_URL=https://artifactory.eu02.arm.com:443/artifactory/mcu.promoted
else
    ARTIFACTORY_URL=https://eu-west-1.artifactory.aws.arm.com:443/artifactory/mcu.promoted
fi

if [ -z "$ARTIFACTORY_API_KEY" ]; then
  echo "Please set your Artifactory in ARTIFACTORY_API_KEY"
  echo ""
  echo "1. Browse to $(dirname $(dirname $ARTIFACTORY_URL))/ui/admin/artifactory/user_profile"
  echo "2. Copy the API Key"
  echo "3. Add 'export ARTIFACTORY_API_KEY=\"<API Key>\"' to ~/.bashrc"
  exit 1
fi

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

pushd $(dirname $0) > /dev/null

ARCHIVE_NAME="RTX-${VERSION}.zip"
ARCHIVE_URL="${ARTIFACTORY_URL}/CMSIS_5/Libraries/${ARCHIVE_NAME}"
echo "Fetching ${ARCHIVE_URL}..."

if [[ $FORCE == 1 ]]; then
    rm ${ARCHIVE_NAME}
fi

if [[ -f ${ARCHIVE_NAME} ]]; then
    sha256sum=$(curl -s -I -H "X-JFrog-Art-Api:${ARTIFACTORY_API_KEY}" "${ARCHIVE_URL}" | grep "X-Checksum-Sha256" | cut -d" " -f2)
    if echo "${sha256sum} *${ARCHIVE_NAME}" | sha256sum -c --status; then
        echo "Already up-to-date"
    else
        rm ${ARCHIVE_NAME}
    fi
fi

if [[ ! -f ${ARCHIVE_NAME} ]]; then
    curl -C - -H "X-JFrog-Art-Api:${ARTIFACTORY_API_KEY}" -O "${ARCHIVE_URL}"
fi

unzip -u ${ARCHIVE_NAME}

exit 0
