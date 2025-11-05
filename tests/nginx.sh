#!/bin/bash

configPath="/home/edouard/Projects/webserv/tests/nginx.conf"
configPort=8005

# List of tests to do
tests=(
	"test"
	"test/"
	""
	"/"
	"test/notfound"
	"nonexistent"
	"test/home.html"
)

# Clean and start
sudo lsof -i :$configPort | awk 'NR>1 {print $2}' | xargs -r sudo kill 2>/dev/null
nginx -c $configPath
sleep 0.5
clear

for test in "${tests[@]}"; do
	# Clean extra slashes at the beginning
	test=$(echo "$test" | sed 's|^/||')
	finalUrl="http://localhost:$configPort/$test"

	echo -e "\n--- Testing: /$test ($finalUrl) ---"

	# Do request and get HTTP code
	http_code=$(curl -s -o /dev/null -w "%{http_code}" "$finalUrl")
	echo "Code HTTP: $http_code"

	# Display first 5 lines of response
	echo "Response:"
	curl -s "$finalUrl" | head -n 5
    echo "---"
done

# Final cleanup
nginx -s stop -c $configPath 2>/dev/null || true