#!/bin/bash

echo "stop tsdb begin..."

work_path=$(grep work_path ./config.json | awk -F "\"" '{print $4}')
cat $work_path/pidfile | xargs kill 

echo "stop tsdb end"
