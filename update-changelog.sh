#!/bin/bash

gitlog-to-changelog  | sed  's/turbulence: *//g' > ChangeLog
