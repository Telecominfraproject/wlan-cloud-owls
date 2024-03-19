ARG DEBIAN_VERSION=11.4-slim
ARG POCO_VERSION=poco-tip-v2
ARG CPPKAFKA_VERSION=tip-v1
ARG VALIJASON_VERSION=tip-v1

FROM debian:$DEBIAN_VERSION AS build-base

RUN apt-get update && apt-get install --no-install-recommends -y \
    make cmake g++ git \
    libpq-dev libmariadb-dev libmariadbclient-dev-compat \
    librdkafka-dev libboost-all-dev libssl-dev \
    zlib1g-dev nlohmann-json3-dev ca-certificates libfmt-dev

FROM build-base AS poco-build

ARG POCO_VERSION

ADD https://api.github.com/repos/Telecominfraproject/wlan-cloud-lib-poco/git/refs/tags/${POCO_VERSION} version.json
RUN git clone https://github.com/Telecominfraproject/wlan-cloud-lib-poco --branch ${POCO_VERSION} /poco

WORKDIR /poco
RUN mkdir cmake-build
WORKDIR cmake-build
RUN cmake ..
RUN cmake --build . --config Release -j8
RUN cmake --build . --target install

FROM build-base AS cppkafka-build

ARG CPPKAFKA_VERSION

ADD https://api.github.com/repos/Telecominfraproject/wlan-cloud-lib-cppkafka/git/refs/tags/${CPPKAFKA_VERSION} version.json
RUN git clone https://github.com/Telecominfraproject/wlan-cloud-lib-cppkafka --branch ${CPPKAFKA_VERSION} /cppkafka

WORKDIR /cppkafka
RUN mkdir cmake-build
WORKDIR cmake-build
RUN cmake ..
RUN cmake --build . --config Release -j8
RUN cmake --build . --target install

FROM build-base AS valijson-build

ARG VALIJASON_VERSION

ADD https://api.github.com/repos/Telecominfraproject/wlan-cloud-lib-valijson/git/refs/tags/${VALIJASON_VERSION} version.json
RUN git clone https://github.com/Telecominfraproject/wlan-cloud-lib-valijson --branch ${VALIJASON_VERSION} /valijson

WORKDIR /valijson
RUN mkdir cmake-build
WORKDIR cmake-build
RUN cmake ..
RUN cmake --build . --config Release -j8
RUN cmake --build . --target install

FROM build-base AS owls-build

ADD CMakeLists.txt build /owls/
ADD cmake /owls/cmake
ADD src /owls/src
ADD .git /owls/.git

COPY --from=poco-build /usr/local/include /usr/local/include
COPY --from=poco-build /usr/local/lib /usr/local/lib
COPY --from=cppkafka-build /usr/local/include /usr/local/include
COPY --from=cppkafka-build /usr/local/lib /usr/local/lib
COPY --from=valijson-build /usr/local/include /usr/local/include

WORKDIR /owls
RUN mkdir cmake-build
WORKDIR /owls/cmake-build
RUN cmake ..
RUN cmake --build . --config Release -j8

FROM debian:$DEBIAN_VERSION

ENV OWLS_USER=owls \
    OWLS_ROOT=/owls-data \
    OWLS_CONFIG=/owls-data

RUN useradd "$OWLS_USER"

RUN mkdir /openwifi
RUN mkdir -p "$OWLS_ROOT" "$OWLS_CONFIG" && \
    chown "$OWLS_USER": "$OWLS_ROOT" "$OWLS_CONFIG"

RUN apt-get update && apt-get install --no-install-recommends -y \
    librdkafka++1 gosu gettext ca-certificates bash jq curl wget \
    libmariadb-dev-compat libpq5 unixodbc postgresql-client libfmt7

COPY test_scripts/curl/cli /cli

COPY owls.properties.tmpl /
COPY docker-entrypoint.sh /
COPY wait-for-postgres.sh /
RUN wget https://raw.githubusercontent.com/Telecominfraproject/wlan-cloud-ucentral-deploy/main/docker-compose/certs/restapi-ca.pem \
    -O /usr/local/share/ca-certificates/restapi-ca-selfsigned.crt

COPY --from=owls-build /owls/cmake-build/owls /openwifi/owls
COPY --from=cppkafka-build /cppkafka/cmake-build/src/lib/* /lib/
COPY --from=poco-build /poco/cmake-build/lib/* /lib/

RUN ldconfig

EXPOSE 16007 17007 16107

ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["/openwifi/owls"]
