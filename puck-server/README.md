# README #

Private toy project

# Build HOWTO #

### Build and run server ###

#### Build `br` (written in Go) ####

1. Install Go 1.16 or later
2. Open command prompt
3. Execute: `go build br.go`
4. Open new command prompt
5. Execute: `cd db-server && ../br db`
6. Open new command prompt
7. Execute: `cd reward-server && ../br reward`
8. Open new command prompt
9. Configure `match-server/conf.json` using a template `match-server/conf.json.template`
   -  NOTE: `BattlePublicServiceHost` should be set to public IP of battle server
10. Execute: `cd match-server && ../br match`
11. Open new command prompt
12. Execute: `cd rank-server && ../br rank`
13. [OPTIONAL] Open new command prompt
14. [OPTIONAL] Execute: `cd push-server && ../br push`

CAUTION: Also, you need to build & run `battle-server`. `battle-server` can be built on `../laidoff`. See `../laidoff/README.md`
