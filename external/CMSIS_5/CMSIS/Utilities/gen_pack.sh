#!/bin/bash
# Version: 1.5
# Date: 2022-04-06
# This bash script generates a CMSIS Software Pack:
#
# Pre-requisites:
# - bash shell (for Windows: install git for Windows)
# - git in path (for Windows: install git for Windows)
# - 7z in path (zip archiving utility)
#   e.g. Ubuntu: sudo apt-get install p7zip-full p7zip-rar)
# - xmllint in path (XML schema validation; available only for Linux)
#
# Preparation steps:
# - Generate documentation, see CMSIS/DoxyGen/gen_doc.sh
# - Populate utilities, see
#   - CMSIS/Utilities/fetch_devtools.sh
# - Populate pre-built libraries, see
#   - CMSIS/RTOS/RTX/LIB/fetch_libs.sh
#   - CMSIS/RTOS2/RTX/Library/fetch_libs.sh
#

############### EDIT BELOW ###############
# Extend Path environment variable locally
#

set -o pipefail

function usage {
  echo "$(basename $0) [-h|--help] [<pdsc>]"
  echo ""
  echo "Arguments:"
  echo "  -h|--help        Print this usage message and exit."
  echo "  --ignore_errors  Ignore errors detected during pack generation."
  echo "  --version <VER>  Force pack version to <VER>."
  echo "  pdsc             The pack description to generate the pack for."
  echo ""
  echo "Environment:"
  echo " 7z"
  if [ $(uname -s) = "Linux" ]; then
    echo " xmllint"
  fi
  echo ""
}

function pack_version()
{
  local version=$(grep -Pzo "(?s)<releases>\s+<release version=\"([^\"]+)\"" "$1" | tr -d '\0' | tail -n 1 | sed -r -e 's/.*version="([^"]+)"/\1/g')
  echo "PDSC version: '$version'" >&2
  echo $version
}

function git_describe()
{
  if git rev-parse --git-dir 2>&1 >/dev/null; then
    local gitversion=$(git describe --tags --match $1* --abbrev=9 2>/dev/null || echo "$1-dirty-0-g$(git describe --tags --match $1* --always --abbrev=9 2>/dev/null)")
    local version=$(echo $gitversion | sed -r -e 's/-([0-9]+)-(g[0-9a-f]{9})/\1+\2/')
    if [[ $version != $1 ]] && [[ $version == $gitversion ]]; then
        version+=0
    fi
    echo "Git version: '$version'" >&2
    echo $version
  else
    echo "No Git repository: '$1-nogit'" >&2
    echo "$1-nogit"
  fi
}

function patch_pdsc()
{
  if [[ "$2" != "$3" ]]; then
    echo "Updating latest release tag with version '$3'"
    sed -r -i -e "s/<release version=\"$2\"/<release version=\"$3\"/" $1
  fi
}

IGNORE_ERRORS=0
VERSION=
POSITIONAL=()
while [[ $# -gt 0 ]]
do
  key="$1"

  case $key in
    '-h'|'--help')
      usage
      exit 1
    ;;
    '--ignore-errors')
      IGNORE_ERRORS=1
      shift # past argument
    ;;
    '--version')
      shift # past argument
      VERSION=$1
      shift # past argument
    ;;
    *)    # unknown option
      POSITIONAL+=("$1") # save it in an array for later
      shift # past argument
    ;;
  esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

OS=$(uname -s)
case $OS in
  'Linux')
    CMSIS_TOOLSDIR="./CMSIS/Utilities/Linux64"
    ;;
  'WindowsNT'|MINGW*|CYGWIN*)
    CMSIS_TOOLSDIR="./CMSIS/Utilities/Win32"
    ;;
  'Darwin')
    CMSIS_TOOLSDIR="./CMSIS/Utilities/Darwin64"
    ;;
  *)
    echo "Error: unrecognized OS $OS"
    exit 1
    ;;
esac

PATH_TO_ADD="$CMSIS_TOOLSDIR"

[[ ":$PATH:" != *":${PATH_TO_ADD}:"* ]] && PATH="${PATH}:${PATH_TO_ADD}"
echo $PATH_TO_ADD appended to PATH
echo " "

# Pack warehouse directory - destination
PACK_WAREHOUSE=./output

# Temporary pack build directory
PACK_BUILD=./build

# Specify directory names to be added to pack base directory
PACK_DIRS="
  Device
  CMSIS/Core/Include
  CMSIS/Core/Template
  CMSIS/Core_A
  CMSIS/DAP
  CMSIS/Driver
  CMSIS/RTOS
  CMSIS/RTOS2
  CMSIS/Utilities/Win32
  CMSIS/Utilities/Linux64
  CMSIS/Utilities/Darwin64
  CMSIS/Documentation
"

# Specify file names to be added to pack base directory
PACK_BASE_FILES="
  LICENSE.txt
  CMSIS/Utilities/ARM_Example.*
  CMSIS/Utilities/*.xsd
"

# Specify file names to be deleted from pack build directory
PACK_DELETE_FILES="
  CMSIS/RTOS/CMSIS_RTOS_Tutorial.pdf
  CMSIS/RTOS/RTX/LIB/fetch_libs.sh
  CMSIS/RTOS/RTX/LIB/*.zip
  CMSIS/RTOS2/RTX/Library/fetch_libs.sh
  CMSIS/RTOS2/RTX/Library/*.zip
  CMSIS/RTOS2/RTX/Library/build.py
"

# Specify patches to be applied
PACK_PATCH_FILES=""

############ DO NOT EDIT BELOW ###########
echo Starting CMSIS-Pack Generation: `date`
# Zip utility check
ZIP=7z
type -a "${ZIP}"
errorlevel=$?
if [ $errorlevel -gt 0 ]
  then
  echo "Error: No 7zip Utility found"
  echo "Action: Add 7zip to your path"
  echo " "
  exit 1
fi

# Pack checking utility check
PACKCHK=packchk
type -a ${PACKCHK}
errorlevel=$?
if [ $errorlevel != 0 ]; then
  echo "Error: No packchk Utility found"
  echo "Action: Add packchk to your path"
  echo "Hint: Included in CMSIS Pack:"
  echo "$CMSIS_PACK_ROOT/ARM/CMSIS/<version>/CMSIS/Utilities/<os>/"
  echo " "
  if [[ $IGNORE_ERRORS == 0 ]]; then
    exit 1
  fi
fi
echo " "

# Locate Package Description file
# check whether there is more than one pdsc file
NUM_PDSCS=$(ls -1 *.pdsc | wc -l)
PACK_DESCRIPTION_FILE=$(ls *.pdsc)
if [[ -n $1 && -f $1 ]]; then
  PACK_DESCRIPTION_FILE=$1
elif [ ${NUM_PDSCS} -lt 1 ]; then
  echo "Error: No *.pdsc file found in current directory"
  echo " "
  exit 1
elif [ ${NUM_PDSCS} -gt 1 ]; then
  echo "Error: Only one PDSC file allowed in directory structure:"
  echo "Found:"
  echo "$PACK_DESCRIPTION_FILE"
  echo "Action: Provide PDSC file explicitly!"
  echo " "
  usage
  exit 1
fi

SAVEIFS=$IFS
IFS=.
set ${PACK_DESCRIPTION_FILE}
# Pack Vendor
PACK_VENDOR=$1
# Pack Name
PACK_NAME=$2
echo "Generating Pack: for $PACK_VENDOR.$PACK_NAME"
echo " "
IFS=$SAVEIFS

#if $PACK_BUILD directory does not exist, create it.
if [ ! -d "$PACK_BUILD" ]; then
  mkdir -p "$PACK_BUILD"
fi

# Copy files into build base directory: $PACK_BUILD
# pdsc file is mandatory in base directory:
cp -f "./${PACK_VENDOR}.${PACK_NAME}.pdsc" "${PACK_BUILD}"

# Add directories
echo Adding directories to pack:
echo "${PACK_DIRS}"
echo " "
for d in ${PACK_DIRS}; do
  cp -r --parents "$d" "${PACK_BUILD}"
done

# Add files
echo Adding files to pack:
echo "${PACK_BASE_FILES}"
echo " "
if [ ! -x ${PACK_BASE_FILES+x} ]; then
  for f in ${PACK_BASE_FILES}; do
    cp -f --parents "$f" $PACK_BUILD/
  done
fi

# Delete files
echo Deleting files from pack:
echo "${PACK_DELETE_FILES}"
echo " "
if [ ! -x ${PACK_DELETE_FILES+x} ]; then
  for f in ${PACK_DELETE_FILES}; do
    find $PACK_BUILD/$(dirname "$f") -name $(basename "$f") -delete
  done
fi

# Apply patches
echo Applying patches to pack:
echo "${PACK_PATCH_FILES}"
echo " "
if [ ! -x ${PACK_PATCH_FILES+x} ]; then
  CWD=$(pwd)
  pushd $PACK_BUILD > /dev/null
  for f in ${PACK_PATCH_FILES}; do
    patch -p0 -t -i "${CWD}/${f}"
  done
  popd > /dev/null
fi

# Create checksum file
echo Creating checksum file:
pushd $PACK_BUILD > /dev/null
find . -type f -exec sha1sum {} + > ../${PACK_VENDOR}.${PACK_NAME}.sha1
mv ../${PACK_VENDOR}.${PACK_NAME}.sha1 .
popd > /dev/null

# Run Schema Check (for Linux only):
# sudo apt-get install libxml2-utils

if [ $(uname -s) = "Linux" ]; then
  echo "Running schema check for ${PACK_VENDOR}.${PACK_NAME}.pdsc"
  curl https://raw.githubusercontent.com/Open-CMSIS-Pack/Open-CMSIS-Pack-Spec/main/schema/PACK.xsd -o CMSIS/Utilities/PACK.xsd
  xmllint --noout --schema "$(realpath -m ./CMSIS/Utilities/PACK.xsd)" "${PACK_BUILD}/${PACK_VENDOR}.${PACK_NAME}.pdsc"
  errorlevel=$?
  if [ $errorlevel -ne 0 ]; then
    echo "build aborted: Schema check of $PACK_VENDOR.$PACK_NAME.pdsc against PACK.xsd failed"
    echo " "
    if [[ $IGNORE_ERRORS == 0 ]]; then
        exit 1
    fi
  fi
else
  echo "Use MDK PackInstaller to run schema validation for $PACK_VENDOR.$PACK_NAME.pdsc"
fi

# Patch pack version
echo "Checking PDCS version against Git..."
pdsc_version=$(pack_version "${PACK_BUILD}/${PACK_VENDOR}.${PACK_NAME}.pdsc")
if [ -z $VERSION ]; then
  VERSION=$(git_describe ${pdsc_version})
fi
patch_pdsc "${PACK_BUILD}/${PACK_VENDOR}.${PACK_NAME}.pdsc" ${pdsc_version} ${VERSION}

# Run Pack Check and generate PackName file with version
"${PACKCHK}" "${PACK_BUILD}/${PACK_VENDOR}.${PACK_NAME}.pdsc" \
  -n ${PACK_BUILD}/PackName.txt \
  -x M353 -x M364 -x M335 -x M336
errorlevel=$?
if [ $errorlevel -ne 0 ]; then
  echo "build aborted: pack check failed"
  echo "Check preparation steps if missing files are reported!"
  echo " "
  if [[ $IGNORE_ERRORS == 0 ]]; then
    exit 1
  fi
fi

PACKNAME=$(cat ${PACK_BUILD}/PackName.txt)
rm -rf ${PACK_BUILD}/PackName.txt

# Archiving
# $ZIP a $PACKNAME
echo "creating pack file $PACKNAME"
#if $PACK_WAREHOUSE directory does not exist create it
if [ ! -d "$PACK_WAREHOUSE" ]; then
  mkdir -p "$PACK_WAREHOUSE"
fi
pushd "$PACK_WAREHOUSE" > /dev/null
PACK_WAREHOUSE=$(pwd)
popd  > /dev/null
pushd "$PACK_BUILD" > /dev/null
PACK_BUILD=$(pwd)
"$ZIP" a "$PACK_WAREHOUSE/$PACKNAME" -tzip
popd  > /dev/null
errorlevel=$?
if [ $errorlevel -ne 0 ]; then
  echo "build aborted: archiving failed"
  exit 1
fi

echo "build of pack succeeded"
# Clean up
echo "cleaning up ..."

rm -rf "$PACK_BUILD"
echo " "

echo Completed CMSIS-Pack Generation: $(date)
