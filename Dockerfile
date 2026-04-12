FROM gcc:latest

# Install libraries
RUN apt-get update && apt-get install -y libcurl4-openssl-dev

WORKDIR /app

# 1. Copy the contents of the MyCppBackend folder INTO the app folder
COPY MyCppBackend/ .

# 2. List the files (so we can see it worked in the logs)
RUN ls -la

# 3. Now g++ will see main.cpp right there in front of it
RUN g++ main.cpp -o server -lcurl -lpthread

EXPOSE 8080

CMD ["./server"]