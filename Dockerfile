# Use a Linux image with GCC installed
FROM gcc:latest

# Install libcurl (required for your Google/Groq searches)
RUN apt-get update && apt-get install -y libcurl4-openssl-dev

# Set the working directory
WORKDIR /app

# Copy your C++ code into the cloud server
COPY . .

# Compile the code (Using Linux flags)
RUN g++ main.cpp -o server -lcurl -lpthread

# Tell the server to listen on port 8080
EXPOSE 8080

# Run the server
CMD ["./server"]