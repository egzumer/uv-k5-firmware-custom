#!/bin/bash

function usage {
  echo "$(basename $0) <file>"
  echo ""
  echo "Arguments:"
  echo "  -h|--help         Show this usage text."
  echo "  -v|--verbose      Print verbose output."
  echo "  -d|--debug        Print debug output."
  echo "  -b|--base <sha>   Git commit SHA of merge base."
  echo "  <file>            The file to check the header."
  echo ""
}

function echo-verbose {
  if [[ $VERBOSE != 0 ]]; then
    echo $1
  fi
}

function echo-debug {
  if [[ $DEBUG != 0 ]]; then
    echo $1
  fi
}

set -o pipefail

VERBOSE=0
DEBUG=0
BASE_REV=""
POSITIONAL=()
while [[ $# -gt 0 ]]
do
  key="$1"

  case $key in
    '-h'|'--help')
      usage
      exit 1
    ;;
    '-v'|'--verbose')
      VERBOSE=1
    ;;
    '-d'|'--debug')
      DEBUG=1
    ;;
    '-b'|'--base')
      shift
      if git rev-parse $1 2>/dev/null >/dev/null; then
        BASE_REV=$(git rev-parse $1)
      else
        echo "Unknown revision: $1" >&2
      fi
    ;;
    *)    # unknown option
      POSITIONAL+=("$1") # save it in an array for later
    ;;
  esac
  shift # past argument
done
set -- "${POSITIONAL[@]}" # restore positional parameters

if [[ -z "$1" ]] || [[ ! -f $1 ]]; then
  echo -e "No file given!\n"
  usage
  exit 1
fi

FILE=$1
RESULT=0

echo "Checking $1"

echo-debug "grep -E '@date\s+([0-9]{2}\. \w+ [0-9]{4})' ${FILE} | sed -E 's/^.*@date\s+([0-9]{2}\. \w+ [0-9]{4}).*/\1/'"
FILE_DATE=$(grep -E '@date\s+([0-9]{2}\. \w+ [0-9]{4})' ${FILE} | sed -E 's/^.*@date\s+([0-9]{2}\. \w+ [0-9]{4}).*/\1/')
echo-verbose "File date: $FILE_DATE"
if [[ ! -z $FILE_DATE ]]; then
  echo-debug "git log -1 --pretty="format:%ad" --date="format:%d. %B %Y" ${FILE}"
  HEAD_DATE=$(git log -1 --pretty="format:%ad" --date="format:%d. %B %Y" ${FILE})
  echo-verbose "Head date: $HEAD_DATE"
  if [[ $HEAD_DATE != $FILE_DATE ]]; then
    echo-debug "grep -En "@date.*${FILE_DATE}" ${FILE} | cut -f1 -d:"
    FILE_DATE_LINE=$(grep -En "@date.*${FILE_DATE}" ${FILE} | cut -f1 -d:)
    echo "${FILE}:${FILE_DATE_LINE}:Please update file date to '$HEAD_DATE'." >&2
    RESULT=1
  fi
fi

echo-debug "grep -E '@version\s+V?([0-9]+\.[0-9]+(\.[0-9]+)?)' ${FILE} | sed -E 's/^.*@version\s+V?([0-9]+\.[0-9]+(\.[0-9]+)?).*/\1/'"
FILE_VERSION=$(grep -E '@version\s+V?([0-9]+\.[0-9]+(\.[0-9]+)?)' ${FILE} | sed -E 's/^.*@version\s+V?([0-9]+\.[0-9]+(\.[0-9]+)?).*/\1/')
echo-verbose "File version: $FILE_VERSION"
if [[ ! -z $FILE_VERSION ]]; then
  echo-debug "grep -En \"@version.*${FILE_VERSION}\" ${FILE} | cut -f1 -d:"
  FILE_VERSION_LINE=$(grep -En "@version.*${FILE_VERSION}" ${FILE} | cut -f1 -d:)
  echo-verbose "File version line: $FILE_VERSION_LINE"
  echo-debug "git log -1 --pretty=\"format:%H\" -- ${FILE}"
  HEAD_REV=$(git log -1 --pretty="format:%H" -- ${FILE})
  echo-verbose "Head revision : $HEAD_REV"
  if [[ -z "$BASE_REV" ]] || [[ $HEAD_REV =~ ^$BASE_REV ]]; then
    echo-debug "git log -1 --pretty=\"format:%P\" -- ${FILE}"
    BASE_REV=$(git log -1 --pretty="format:%P" -- ${FILE})
  fi
  echo-verbose "Base revision : $BASE_REV"
  echo-debug "git blame ${BASE_REV}..${HEAD_REV} -l -L ${FILE_VERSION_LINE},${FILE_VERSION_LINE} ${FILE}"
  BLAME=$(git blame ${BASE_REV}..${HEAD_REV} -l -L ${FILE_VERSION_LINE},${FILE_VERSION_LINE} ${FILE})
  echo-debug "git rev-parse $(sed -E 's/^[\^]?([[:alnum:]]+).*/\1/' <<<$BLAME)"
  BLAME_REV=$(git rev-parse $(sed -E 's/^[\^]?([[:alnum:]]+).*/\1/' <<<$BLAME))
  echo-verbose "Blame revision: $BLAME_REV"
  if [[ $BASE_REV == $BLAME_REV ]]; then
    echo "${FILE}:${FILE_VERSION_LINE}:Please increment file version." >&2
    RESULT=1
  fi
fi

exit $RESULT
