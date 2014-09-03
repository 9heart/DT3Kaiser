#!/bin/sh

echo ----data.pack----

./PackageMaker ./data.pack \
-compress lvl \
-compress mat \
-compress shdr \
-compress txt \
./data/editor \
./data/fonts \
./data/levels \
./data/localization \
./data/materials \
./data/shaders \
./data/sounds

./PackageMaker ./data_ads.pack \
-compress lvl \
-compress mat \
-compress shdr \
-compress txt \
./data_ads

