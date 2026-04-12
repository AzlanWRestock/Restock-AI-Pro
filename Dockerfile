FROM gcc:latest

# 1. Install curl AND the asio development library
RUN apt-get update && apt-get install -y \
    libcurl4-openssl-dev \
    libasio-dev

WORKDIR /app

# 2. Copy your backend files
COPY MyCppBackend/ .

# 3. List files to verify
RUN ls -la

# 4. Compile with the correct flags
# We added -I/usr/include/asio to make sure it finds the headers
RUN g++ main.cpp -o server -lcurl -lpthread

EXPOSE 8080

CMD ["./server"]