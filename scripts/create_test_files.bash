#!/bin/bash
scripts_dir=$(dirname "$0")
repo_dir=$(dirname "$scripts_dir")

zcat -f $repo_dir/sample-data.txt.gz > "$scripts_dir/samples.txt"
zcat -f $repo_dir/sample-data.txt.gz | grep "^+" | "$repo_dir/uat2text" > "$scripts_dir/uplink_tests.txt"
zcat -f $repo_dir/sample-data.txt.gz | grep "^-" | "$repo_dir/uat2text" > "$scripts_dir/downlink_tests.txt"