# Stage 1: Install system dependencies and vcpkg
FROM ubuntu:22.04 AS dependencies
RUN export DEBIAN_FRONTEND=noninteractive \
	&& ln -fs /usr/share/zoneinfo/America/Sao_Paulo /etc/localtime

RUN apt-get update && apt-get install -y --no-install-recommends \
	git cmake curl zip unzip tar automake ca-certificates build-essential \
	libglew-dev libx11-dev autoconf libtool pkg-config tzdata libssl3 \
	&& dpkg-reconfigure --frontend noninteractive tzdata \
	&& apt-get clean && apt-get autoclean

WORKDIR /opt
COPY vcpkg.json /opt
RUN vcpkgCommitId=$(grep '.builtin-baseline' vcpkg.json | awk -F: '{print $2}' | tr -d '," ') \
	&& echo "vcpkg commit ID: $vcpkgCommitId" \
	&& git clone https://github.com/Microsoft/vcpkg.git \
	&& cd vcpkg \
	&& git checkout $vcpkgCommitId \
	&& ./bootstrap-vcpkg.sh

WORKDIR /opt/vcpkg
COPY vcpkg.json /opt/vcpkg/
RUN /opt/vcpkg/vcpkg --feature-flags=binarycaching,manifests,versions install


# Stage 2: Build the project executable
FROM dependencies AS builder
COPY ./ /otclient/
WORKDIR /otclient/build
RUN cmake -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_CXX_FLAGS="-w" ..
RUN make -j$(nproc)


# Stage 3: Create the final image
FROM ubuntu:22.04 AS final
RUN apt-get update; \
 	apt-get install -y \
	libluajit-5.1-dev \
 	libglew-dev \
	libx11-dev \
 	libopenal1 \
 	libopengl0 \
 	&& apt-get clean && apt-get autoclean

WORKDIR /otclient
COPY --from=builder /otclient/build/otclient /otclient
COPY ./data/ /otclient/data/.
COPY ./layouts/ /otclient/layouts/.
COPY ./mods/ /otclient/mods/.
COPY ./modules/ /otclient/modules/.
COPY ./init.lua /otclient/.
WORKDIR /otclient
CMD ["./otclient"]
