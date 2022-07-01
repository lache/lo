package config

type ServerConfig struct {
	ConnHost string
	ConnPort string
	ConnType string

	BattleServiceHost     string
	BattleServicePort     string
	BattleServiceConnType string

	BattlePublicServiceHost     string
	BattlePublicServicePort     string
	BattlePublicServiceConnType string
}
