#!/bin/bash

PID=$(ps aux | awk '$11 ~ "./main" { print $2 }')

kill -9 $PID
