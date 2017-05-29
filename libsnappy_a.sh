#!/bin/bash

echo -n `dpkg -L libsnappy-dev | grep libsnappy.a`
