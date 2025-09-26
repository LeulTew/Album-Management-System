# syntax=docker/dockerfile:1

FROM gcc:12 AS build
WORKDIR /app
COPY . /app
RUN apt-get update \
    && apt-get install -y --no-install-recommends cmake ninja-build \
    && rm -rf /var/lib/apt/lists/* \
    && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

FROM debian:bookworm-slim
WORKDIR /app
RUN apt-get update \
    && apt-get install -y --no-install-recommends libstdc++6 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=build /app/build/album_management /usr/local/bin/album_management
ENTRYPOINT ["album_management"]
