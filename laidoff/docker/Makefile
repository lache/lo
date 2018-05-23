build:
	./batch-build-servers.sh && \
		docker build -t br .

run-local:
	(docker kill br-run-local || true) && \
	(docker rm br-run-local || true) && \
	docker run -d \
		-p 10288:10288/udp \
		-p 18080:18080 \
		-p 19856:19856 \
		-p 20182:20182 \
		-e "BATTLE_HOST=127.0.0.1" \
		-v '/etc/ssl/certs/ca-certificates.crt:/etc/ssl/certs/ca-certificates.crt' \
		-v br:/data \
		--name br-run-local \
		br

run:
	(docker kill br-run || true) && \
	(docker rm br-run || true) && \
	docker run -d \
		-p 10288:10288/udp \
		-p 18080:18080 \
		-p 19856:19856 \
		-p 20182:20182 \
		-e "BATTLE_HOST=`dig +short myip.opendns.com @resolver1.opendns.com`" \
		-v '/etc/ssl/certs/ca-certificates.crt:/etc/ssl/certs/ca-certificates.crt' \
		-v br:/data \
		--name br-run \
		br

