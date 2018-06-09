#!/bin/bash

rm -rf bin
rm -rf services

mkdir bin
mkdir services

# laidoff-server (battle-server)
mkdir build-server
cd build-server
CXXFLAGS="-static -Wl,-Bdynamic,-lgcc_s,-Bstatic" cmake ../.. -DSERVER_ONLY=1
make -j8 laidoff-server
cp bin/laidoff-server ../bin/
cd ..

# br-server (all-in-one server binary)
TMP_GOPATH="/tmp/gopath"
rm -rf "${TMP_GOPATH}"
GITHUB_PATH="$(git remote -v | grep "$(cat ../.author)" | head -n1 | cut -d"@" -f2 | cut -d"." -f1-2 | cut -d" " -f1 | tr ":" "/")"
TMP_PROJECT_PATH="${TMP_GOPATH}/src/${GITHUB_PATH}"
TMP_USER_ROOT="$(dirname "${TMP_PROJECT_PATH}")"

BUILD_PATH="$(pwd)"
PROJECT_PATH="$(dirname "${BUILD_PATH}")"
SYMLINK_SOURCE_PATH="$(dirname "${PROJECT_PATH}")" # go up one level more
if [ ! -d "${TMP_PROJECT_PATH}" ]; then
  echo "Creating '${TMP_USER_ROOT}'..."
  mkdir -p "${TMP_USER_ROOT}"
  echo "Symbolic linking '${SYMLINK_SOURCE_PATH}' to '${TMP_PROJECT_PATH}'..."
  ln -s "${SYMLINK_SOURCE_PATH}" "${TMP_PROJECT_PATH}"
fi

cd "${TMP_PROJECT_PATH}/laidoff"
GOPATH="${TMP_GOPATH}" go get ./br-server
if [ "$?" -ne 0 ]; then
  echo "Your go is broken!"
  exit 1
fi

GOPATH="${TMP_GOPATH}" CC="$(which musl-gcc)" go build --ldflags '-w -linkmode external -extldflags "-static"' -o "${BUILD_PATH}/bin/br-server" ./br-server/br.go
cd -

cd services

# laidoff-server
mkdir battle

# db-server
mkdir db
# db-server resources
cp ../../db-server/*.txt db
cp ../../db-server/*.html db

# match-server
mkdir match
# match-server resources
cp ../../match-server/conf.json.template match/conf.json

# rank-server
mkdir rank
# rank-server resources
# ---

# reward-server
mkdir reward
# reward-server resources
# ---

# push-server
mkdir push
# push-server resources
cp ../../push-server/*.html push

cd ..

