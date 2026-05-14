#!/bin/bash

# Configuration
# Make sure this matches the AUTH_TOKEN environment variable you run the server with
AUTH_TOKEN=${AUTH_TOKEN:-"SOME_RANDOM_TOKEN"}
SERVER_URL="http://localhost:8080/process"


echo 
if [ $# -eq 1 ]; then
    IMAGE_PATH="$1"
else
    IMAGE_PATH="another_test.png"
fi

# Create a dummy file for testing if one doesn't exist
if [ ! -f "$IMAGE_PATH" ]; then
    echo "Creating dummy file: $IMAGE_PATH"
    echo "dummy image content" > "$IMAGE_PATH"
fi

echo "Sending POST request to $SERVER_URL..."
echo "Using token: $AUTH_TOKEN"
echo "----------------------------------------"

# Send the request using curl
# -X POST : Use POST method
# -H "Authorization: ..." : Add the Bearer token header
# -F "image=@..." : Send as multipart/form-data with a file attachment
curl -X POST "$SERVER_URL" \
     -H "Authorization: Bearer $AUTH_TOKEN" \
     -F "image=@$IMAGE_PATH" \
     -w "\n\nHTTP Status: %{http_code}\n"
