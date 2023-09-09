#!/bin/bash
# Version: 1.0
# Date: 2021-05-05
# This bash script generates CMSIS Documentation:
#
# Pre-requisites:
# - bash shell (for Windows: install git for Windows)
# - doxygen 1.8.6
# - mscgen 0.20

set -o pipefail

DIRNAME=$(dirname $(readlink -f $0))
DOXYGEN=$(which doxygen)
MSCGEN=$(which mscgen)
REGEN=0
ALLPARTS=($(find ${DIRNAME} -mindepth 1 -maxdepth 1 -type d -exec basename {} \;))
PARTS=()

if [[ -z "$*" ]]; then
    REGEN=1
else
    for part in "$*"; do
        if [[ " ${ALLPARTS[@]} " =~ " $part " ]]; then
            PARTS+=($part)
        fi
    done
fi

if [[ ! -f "${DOXYGEN}" ]]; then
    echo "Doxygen not found!" >&2
    echo "Did you miss to add it to PATH?"
    exit 1
else
    version=$("${DOXYGEN}" --version)
    echo "DOXYGEN is ${DOXYGEN} at version ${version}"
    if [[ "${version}" != "1.8.6" ]]; then
        echo " >> Version is different from 1.8.6 !" >&2
    fi
fi

if [[ ! -f "${MSCGEN}" ]]; then
    echo "mscgen not found!" >&2
    echo "Did you miss to add it to PATH?"
    exit 1
else
    version=$("${MSCGEN}" 2>/dev/null | grep "Mscgen version" | sed -r -e 's/Mscgen version ([^,]+),.*/\1/')
    echo "MSCGEN is ${MSCGEN} at version ${version}"
    if [[ "${version}" != "0.20" ]]; then
        echo " >> Version is different from 0.20 !" >&2
    fi
fi

function doxygen {
    partname=$(basename $(dirname $1))
    if [[ $REGEN != 0 ]] || [[ " ${PARTS[@]} " =~ " ${partname} " ]]; then
        pushd "$(dirname $1)" > /dev/null
        echo "${DOXYGEN} $1"
        "${DOXYGEN}" $(basename "$1")
        popd > /dev/null

        if [[ $2 != 0 ]]; then
            cp -f "${DIRNAME}/Doxygen_Templates/search.css" "${DIRNAME}/../Documentation/${partname}/html/search/"
        fi

        projectName=$(grep -E "PROJECT_NAME\s+=" $1 | sed -r -e 's/[^"]*"([^"]+)"/\1/')
        projectNumber=$(grep -E "PROJECT_NUMBER\s+=" $1 | sed -r -e 's/[^"]*"([^"]+)"/\1/')
        datetime=$(date -u +'%a %b %e %Y %H:%M:%S')
        sed -e "s/{datetime}/${datetime}/" "${DIRNAME}/Doxygen_Templates/cmsis_footer.js" \
          | sed -e "s/{projectName}/${projectName}/" \
          | sed -e "s/{projectNumber}/${projectNumber}/" \
          > "${DIRNAME}/../Documentation/${partname}/html/cmsis_footer.js"
    fi
}

if [[ $REGEN != 0 ]]; then
    echo "Cleaning existing documentation ..."
    find "${DIRNAME}/../Documentation/" -mindepth 1 -maxdepth 1 -type d -exec rm -rf {} +
fi

echo "Generating documentation ..."
echo "Copying Build html"
mkdir -p "${DIRNAME}/../Documentation/Build/"
cp -r "${DIRNAME}/Build/html/" "${DIRNAME}/../Documentation/Build/"
doxygen "${DIRNAME}/Core/core.dxy" 1
doxygen "${DIRNAME}/Core_A/core_A.dxy" 1
doxygen "${DIRNAME}/DAP/dap.dxy" 1
doxygen "${DIRNAME}/Driver/Driver.dxy" 1
echo "Copying DSP html"
mkdir -p "${DIRNAME}/../Documentation/DSP/"
cp -r "${DIRNAME}/DSP/html" "${DIRNAME}/../Documentation/DSP/"
doxygen "${DIRNAME}/General/general.dxy" 0
doxygen "${DIRNAME}/DAP/dap.dxy" 1
echo "Copying NN html"
mkdir -p "${DIRNAME}/../Documentation/NN/"
cp -r "${DIRNAME}/NN/html" "${DIRNAME}/../Documentation/NN/"
echo "Copying Pack html"
mkdir -p "${DIRNAME}/../Documentation/Pack/"
cp -r "${DIRNAME}/Pack/html" "${DIRNAME}/../Documentation/Pack/"
doxygen "${DIRNAME}/RTOS/rtos.dxy" 1
doxygen "${DIRNAME}/RTOS2/rtos.dxy" 1
doxygen "${DIRNAME}/SVD/svd.dxy" 0
doxygen "${DIRNAME}/Zone/zone.dxy" 1

exit 0
