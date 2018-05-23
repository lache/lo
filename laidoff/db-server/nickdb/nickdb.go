package nickdb

import (
	"os"
	"bufio"
	"log"
	"fmt"
	"strings"
	"math/rand"
)

func loadFileLines(path string) ([]string, int, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, 0, err
	}
	defer file.Close()
	var lines []string
	scanner := bufio.NewScanner(file)
	maxLen := 0
	for scanner.Scan() {
		newLine := scanner.Text()
		newLineLen := len(newLine)
		if maxLen < newLineLen {
			maxLen = newLineLen
		}
		lines = append(lines, scanner.Text())
	}
	return lines, maxLen, scanner.Err()
}

type NickDb struct {
	adjDb []string
	adjMaxLen int
	nounDb []string
	nounMaxLen int
}

func LoadNickDbByFilename(adjFilename, nounFilename string) NickDb {
	adjDb, adjMaxLen, err := loadFileLines(adjFilename)
	if err != nil {
		log.Fatalf("adj load error: %v", err)
	}
	log.Printf("%v adjs loaded. (max length element: %v)", len(adjDb), adjMaxLen)

	nounDb, nounMaxLen, err := loadFileLines(nounFilename)
	if err != nil {
		log.Fatalf("noun load error: %v", err)
	}
	log.Printf("%v nouns loaded. (max length element: %v)", len(nounDb), nounMaxLen)
	return NickDb{
		adjDb,
		adjMaxLen,
		nounDb,
		nounMaxLen,
	}
}

func PickRandomNick(ndb *NickDb) string {
	adj := ndb.adjDb[rand.Intn(len(ndb.adjDb))]
	noun := ndb.nounDb[rand.Intn(len(ndb.nounDb))]
	return fmt.Sprintf("%s%s", strings.Title(adj), strings.Title(noun))
}
