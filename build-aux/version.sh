#!/bin/bash

exec git describe --abbrev=4 HEAD | sed -e 's/^v//' -e 's/-/./' -e 's/-g/-/'
