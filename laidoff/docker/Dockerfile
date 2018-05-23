FROM alpine:3.7
#RUN apk add --no-cache bash
WORKDIR /app
ADD ./scripts /app/scripts
ADD ./bin /app/bin
ADD ./services /app/services
# match-server (Lobby) TCP port
EXPOSE 19856
# laidoff-server (Battle) UDP port
EXPOSE 10288/udp
# db-server admin TCP port
EXPOSE 20182
# push-server admin TCP port
EXPOSE 18080
CMD ["./scripts/runall.sh"]

