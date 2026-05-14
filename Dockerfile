# Stage 1: Build
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy CMake configuration and source code
COPY CMakeLists.txt .
COPY src/ ./src/

# Configure and build
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Stage 2: Runtime
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    libcurl4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Create directories for output volumes
RUN mkdir -p /app/images /app/data

# Copy binary and template from builder
COPY --from=builder /app/build/server /app/server
COPY --from=builder /app/src/response_template.json /app/response_template.json

# Expose port
EXPOSE 8080

# Run the server
CMD ["/app/server"]
