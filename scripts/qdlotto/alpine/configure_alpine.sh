#!/bin/sh

echo "Install docker"
apk add docker

echo "Start docker on boot"
rc-update add docker default
service docker start
sleep 10

NUM=`docker ps -a -q | wc -l`
if [ ${NUM} -gt 0 ]; then
  echo "Remove all existing docker containers and images"
  docker ps -a -q | xargs -n 1 docker stop
  docker ps -a -q | xargs -n 1 docker rm
  echo y | docker container prune
  echo y | docker image prune
fi

echo "Load image docker-image.tar into docker, might take a while..."
docker load < docker-image.tar

echo "Add autorun commands to /etc/profile"
cat run-script.sh >> /etc/profile
