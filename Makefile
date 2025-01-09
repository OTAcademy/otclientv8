compile:
	docker build -t otclient:latest .
	docker create --name otclient_build otclient:latest
	docker cp otclient_build:/otclient/otclient ./otclient
	docker rm otclient_build

clean:
	docker rm -f otclient_build
	docker system prune -af --volumes

git-clean:
	git fetch --prune
	@for branch in $(shell git branch -vv | grep ': gone]' | awk '{print $$1}'); do \
		git branch -d $$branch || git branch -D $$branch; \
	done
