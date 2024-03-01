#!/bin/bash

./docker-build.sh
docker run -v .:/build -p 8000:8000 landontclipp.github.io serve -a 0.0.0.0:8000