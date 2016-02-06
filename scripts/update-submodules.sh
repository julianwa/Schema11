#!/bin/bash

git submodule sync --recursive
git submodule init
git submodule update --init --recursive
