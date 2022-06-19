FROM alpine:3.15 AS build-base

RUN apk add --update --no-cache \
    make cmake g++ git \
    unixodbc-dev postgresql-dev mariadb-dev \
    librdkafka-dev boost-dev openssl-dev \
    zlib-dev nlohmann-json

FROM build-base AS poco-build

ADD https://api.github.com/repos/AriliaWireless/poco/git/refs/tags/poco-tip-v1 version.json
RUN git clone https://github.com/AriliaWireless/poco --branch poco-tip-v1 /poco

WORKDIR /poco
RUN mkdir cmake-build
WORKDIR cmake-build
RUN cmake ..
RUN cmake --build . --config Release -j8
RUN cmake --build . --target install

FROM build-base AS cppkafka-build

ADD https://api.github.com/repos/AriliaWireless/cppkafka/git/refs/tags/tip-v1 version.json
RUN git clone https://github.com/AriliaWireless/cppkafka --branch tip-v1 /cppkafka

WORKDIR /cppkafka
RUN mkdir cmake-build
WORKDIR cmake-build
RUN cmake ..
RUN cmake --build . --config Release -j8
RUN cmake --build . --target install

FROM build-base AS fmtlib-build

ADD https://api.github.com/repos/fmtlib/fmt/git/refs/tags/9.0.0 version.json
RUN git clone https://github.com/fmtlib/fmt --branch 9.0.0 /fmtlib

WORKDIR /fmtlib
RUN mkdir cmake-build
WORKDIR cmake-build
RUN cmake ..
RUN make
RUN make install

FROM build-base AS json-schema-validator-build

ADD https://api.github.com/repos/pboettch/json-schema-validator/git/refs/heads/master version.json
RUN git clone https://github.com/pboettch/json-schema-validator /json-schema-validator

WORKDIR /json-schema-validator
RUN mkdir cmake-build
WORKDIR cmake-build
RUN cmake ..
RUN make
RUN make install

FROM build-base AS owls-build

ADD CMakeLists.txt build /owls/
ADD cmake /owls/cmake
ADD src /owls/src
ADD .git /owls/.git

COPY --from=poco-build /usr/local/include /usr/local/include
COPY --from=poco-build /usr/local/lib /usr/local/lib
COPY --from=cppkafka-build /usr/local/include /usr/local/include
COPY --from=cppkafka-build /usr/local/lib /usr/local/lib
COPY --from=json-schema-validator-build /usr/local/include /usr/local/include
COPY --from=json-schema-validator-build /usr/local/lib /usr/local/lib
COPY --from=fmtlib-build /usr/local/include /usr/local/include
COPY --from=fmtlib-build /usr/local/lib /usr/local/lib

WORKDIR /owls
RUN mkdir cmake-build
WORKDIR /owls/cmake-build
RUN cmake ..
RUN cmake --build . --config Release -j8

FROM alpine:3.15

ENV OWLS_USER=owls \
    OWLS_ROOT=/owls-data \
    OWLS_CONFIG=/owls-data

RUN addgroup -S "$OWLS_USER" && \
    adduser -S -G "$OWLS_USER" "$OWLS_USER"

RUN mkdir /openwifi
RUN mkdir -p "$OWLS_ROOT" "$OWLS_CONFIG" && \
    chown "$OWLS_USER": "$OWLS_ROOT" "$OWLS_CONFIG"

RUN apk add --update --no-cache librdkafka su-exec gettext ca-certificates bash jq curl \
    mariadb-connector-c libpq unixodbc postgresql-client

COPY test_scripts/curl/cli /cli

COPY owls.properties.tmpl /
COPY docker-entrypoint.sh /
COPY wait-for-postgres.sh /
RUN wget https://raw.githubusercontent.com/Telecominfraproject/wlan-cloud-ucentral-deploy/main/docker-compose/certs/restapi-ca.pem \
    -O /usr/local/share/ca-certificates/restapi-ca-selfsigned.pem

COPY --from=owls-build /owls/cmake-build/owls /openwifi/owls
COPY --from=cppkafka-build /cppkafka/cmake-build/src/lib/* /lib/
COPY --from=poco-build /poco/cmake-build/lib/* /lib/

EXPOSE 16007 17007 16107

ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["/openwifi/owls"]
