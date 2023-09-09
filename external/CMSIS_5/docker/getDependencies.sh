#!/bin/bash

# local variables
DEPENDENCIES_FOLDER=dependenciesFiles
ARTIFACTORY_URL=https://eu-west-1.artifactory.aws.arm.com:443/artifactory
ARTIFACTORY_DEPOT=mcu.depot/ci/depot
PACKCHK_VERSION=1.3.93

if [ -z "$ARTIFACTORY_API_KEY" ]; then
    echo "Please set your Artifactory ARTIFACTORY_API_KEY"
    exit 1
fi

if [ -z "$USER" ]; then
    echo "Please set your short ARM user e.g. sampel01"
    exit 1
fi

function downloadFromArtifactory {
    filename=$(basename $1)
    echo "Fetching ${filename} ..."
    if [[ -f "${filename}" ]]; then
        sha256sum=$(curl -s -I -H "X-JFrog-Art-Api:$ARTIFACTORY_API_KEY" "${ARTIFACTORY_URL}/${1}" | grep "X-Checksum-Sha256" | cut -d" " -f2)
        if echo "${sha256sum} *${filename}" | sha256sum -c --status; then
            echo " ... already up to date"
        else
            rm ${filename}
        fi
    fi
    if [[ ! -f "${filename}" ]]; then
        curl -C - -H "X-JFrog-Art-Api:$ARTIFACTORY_API_KEY" -O "${ARTIFACTORY_URL}/${1}"
        chmod +x ${filename}
    fi
}

function downloadFromDepot {
    downloadFromArtifactory "${ARTIFACTORY_DEPOT}/${1}"
}

function gitClone {
    echo "Cloning/updating ${2} ..."
    if [[ ! -d "${2}" ]]; then
        git clone -b $3 $1 $2
    else
        pushd $2
        git clean -fdx
        git checkout -f $3
        git pull origin $3
        popd
    fi
}

mkdir -p $DEPENDENCIES_FOLDER
pushd $DEPENDENCIES_FOLDER || exit

downloadFromDepot "doxygen_1.8.6-2_amd64.deb"
downloadFromDepot "ArmCompiler-5.06u7-linux.sh"
downloadFromDepot "ArmCompiler-6.16-linux-x86_64.sh"
downloadFromDepot "ArmCompiler-6.6.4-linux-x86_64.sh"
downloadFromDepot "gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2"
downloadFromDepot "fvp-11.12-linux-x86_64.tar.gz"
downloadFromArtifactory "mcu.promoted/staging/devtools/tools/packchk/${PACKCHK_VERSION}/linux64/PackChk"

gitClone "ssh://${USER}@eu-gerrit-1.euhpc.arm.com:29418/dsg/cmsis/buildtools" "buildtools" "master"

popd || exit
