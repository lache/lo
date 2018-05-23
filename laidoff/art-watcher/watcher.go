package main

import (
	"os"
	"log"
	"path/filepath"
	"github.com/fsnotify/fsnotify"
	"sync"
	"time"
	"strings"
	"os/exec"
)

func dataRefreshWatcher(watcher *fsnotify.Watcher, targetDir string) {
	waitMutex := &sync.Mutex{}
	waitMap := make(map[string]chan bool)
	for {
		select {
		case event := <-watcher.Events:
			if event.Op&fsnotify.Write == fsnotify.Write {
				if strings.ToLower(event.Name[len(event.Name)-4:len(event.Name)]) != ".png" {
					continue
				}
				var abortCh chan bool
				waitMutex.Lock()
				// abort previous refresh job
				if prevAbortCh, exist := waitMap[event.Name]; exist {
					prevAbortCh <- true
					abortCh = prevAbortCh
				} else {
					abortCh = make(chan bool, 1)
					waitMap[event.Name] = abortCh
				}
				waitMutex.Unlock()
				go func(abortCh chan bool, filename string) {
					select {
					case <-abortCh:
						// aborted; do nothing
					case <-time.After(1 * time.Second):
						log.Printf("File modified (stable): %v", filename)
						convertFile(filename, targetDir)
						waitMutex.Lock()
						delete(waitMap, filename)
						waitMutex.Unlock()
						close(abortCh)
					}
				}(abortCh, event.Name)
			}
		case err := <-watcher.Errors:
			log.Println("error:", err)
		}
	}
}
func convertFile(filename string, targetDir string) {
	filenameLen := len(filename)
	var convertCmd *exec.Cmd
	if filename[filenameLen-6:filenameLen] == "-a.png" {
		convertCmd = exec.Command(".\\bin\\etcpack.exe", filename, targetDir, "-s", "fast", "-c", "etc1", "-ktx", "-as")
	} else if filename[filenameLen-8:filenameLen] == "-mip.png" {
		convertCmd = exec.Command(".\\bin\\etcpack.exe", filename, targetDir, "-s", "fast", "-c", "etc1", "-ktx", "-mipmaps")
	} else if filename[filenameLen-9:filenameLen] == "-amip.png" {
		convertCmd = exec.Command(".\\bin\\etcpack.exe", filename, targetDir, "-s", "fast", "-c", "etc1", "-ktx", "-as", "-mipmaps")
	} else {
		convertCmd = exec.Command(".\\bin\\etcpack.exe", filename, targetDir, "-s", "fast", "-c", "etc1", "-ktx")
	}
	err := convertCmd.Run()
	if err != nil {
		log.Fatal(err)
	}
}

const (
	ServiceName = "art-watcher"
)

func main() {
	// Set default log format
	log.SetFlags(log.LstdFlags | log.Lshortfile)
	log.SetOutput(os.Stdout)
	log.Printf("Greetings from %v service", ServiceName)
	if len(os.Args) < 3 {
		log.Printf("Should provide resource source directory glob and target directory as first and second argument.")
		return
	}
	sourceDirGlob := os.Args[1]
	targetDir := os.Args[2]
	log.Printf("Source directory glob: %v", sourceDirGlob)
	log.Printf("Target directory: %v", targetDir)
	sourceDirs, err := filepath.Glob(os.Args[1])
	if err != nil {
		log.Fatal(err)
	}
	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		log.Fatal(err)
	}
	for _, f := range sourceDirs {
		log.Printf("Watching source dir %v", f)
		watcher.Add(f)
	}
	done := make(chan bool)
	go dataRefreshWatcher(watcher, targetDir)
	<-done
}
