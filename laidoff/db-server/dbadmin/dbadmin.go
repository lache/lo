package dbadmin

import (
	"regexp"
	"errors"
	"io/ioutil"
	"log"
	"net/http"
	"html/template"
	"github.com/lache/lo/laidoff/db-server/user"
	"strconv"
	"time"
	"sort"
)

type Page struct {
	Title string
	Body  []byte
}

var templates *template.Template
var validPath *regexp.Regexp
//var validPushTokenPath = regexp.MustCompile("^/(savePushToken|editPushToken|deletePushToken|sendTestPush|writePush|sendPush)/([a-zA-Z0-9-:_]+)$")

func initTemplates() {
	templates = template.Must(template.ParseFiles("edit.html", "view.html", "list.html"))
}

func initValidPath() {
	validPath = regexp.MustCompile("^/(edit|save|view)/([a-f0-9\\-]+)$")
}

func getUuidStr(w http.ResponseWriter, r *http.Request) (string, error) {
	m := validPath.FindStringSubmatch(r.URL.Path)
	if m == nil {
		http.NotFound(w, r)
		return "", errors.New("invalid page title")
	}
	return m[2], nil
}

func (p *Page) save() error {
	filename := "pages/" + p.Title + ".txt"
	return ioutil.WriteFile(filename, p.Body, 0600)
}

func loadPage(title string) (*Page, error) {
	filename := "pages/" + title + ".txt"
	body, err := ioutil.ReadFile(filename)
	if err != nil {
		return nil, err
	}
	return &Page{Title: title, Body: body}, nil
}

func renderTemplate(w http.ResponseWriter, tmpl string, p *Page) {
	err := templates.ExecuteTemplate(w, tmpl+".html", p)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func renderUserDbTemplate(w http.ResponseWriter, tmpl string, userDb *user.Db) {
	err := templates.ExecuteTemplate(w, tmpl+".html", userDb)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func viewHandler(w http.ResponseWriter, r *http.Request, title string) {
	title, err := getUuidStr(w, r)
	if err != nil {
		return
	}
	p, err := loadPage(title)
	if err != nil {
		http.Redirect(w, r, "/edit/"+title, http.StatusFound)
		return
	}
	renderTemplate(w, "view", p)
}

func editHandler(w http.ResponseWriter, r *http.Request, uuidStr string) {
	uuidStr, err := getUuidStr(w, r)
	if err != nil {
		return
	}
	userDb, err := user.LoadUserDbByUuidStr(uuidStr)
	if err != nil {
		http.NotFound(w, r)
		return
	}
	renderUserDbTemplate(w, "edit", userDb)
}

func saveHandler(w http.ResponseWriter, r *http.Request, uuidStr string) {
	uuidStr, err := getUuidStr(w, r)
	if err != nil {
		return
	}
	userDb, err := user.LoadUserDbByUuidStr(uuidStr)
	if err != nil {
		http.NotFound(w, r)
		return
	}
	//userDb.Created, err = time.Parse("2017-12-19 17:58:25.3695927 +0900 KST", r.FormValue("Created"))
	//if err != nil {
	//	http.Error(w, err.Error(), http.StatusInternalServerError)
	//	return
	//}
	userDb.Nickname = r.FormValue("Nickname")
	//userDb.LastLogin, err = time.Parse("2017-12-19 17:58:25.3695927 +0900 KST", r.FormValue("LastLogin"))
	//if err != nil {
	//	http.Error(w, err.Error(), http.StatusInternalServerError)
	//	return
	//}
	userDb.WeeklyScore, err = strconv.Atoi(r.FormValue("WeeklyScore"))
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	userDb.Rating, err = strconv.Atoi(r.FormValue("Rating"))
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	userDb.BattleStat.ConsecutiveDefeat, err = strconv.Atoi(r.FormValue("ConsecutiveDefeat"))
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	err = user.WriteUserDb(userDb)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	http.Redirect(w, r, "/list", http.StatusFound)
}

func makeHandler(fn func(http.ResponseWriter, *http.Request, string)) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		m := validPath.FindStringSubmatch(r.URL.Path)
		if m == nil {
			http.NotFound(w, r)
			return
		}
		fn(w, r, m[2])
	}
}

type ListItem struct {
	UuidStr string
	ModTime time.Time
	Db      *user.Db
}

type byModTimeDesc []ListItem

func (a byModTimeDesc) Len() int      { return len(a) }
func (a byModTimeDesc) Swap(i, j int) { a[i], a[j] = a[j], a[i] }
func (a byModTimeDesc) Less(i, j int) bool {
	if a[i].ModTime.Sub(a[j].ModTime) > 0 {
		return true
	}
	return false
}

type ListData struct {
	Title    string
	ListItem []ListItem
}

func listHandler(w http.ResponseWriter, _ *http.Request) {
	files, err := ioutil.ReadDir(user.GetPersistentDbPath())
	if err != nil {
		http.Error(w, "internal error", http.StatusInternalServerError)
	} else {
		listData := ListData{Title: "User List"}
		for _, f := range files {
			userDb, err := user.LoadUserDbByUuidStr(f.Name())
			if err != nil {
				http.Error(w, err.Error(), http.StatusInternalServerError)
			} else {
				listData.ListItem = append(listData.ListItem, ListItem{
					f.Name(),
					f.ModTime(),
					userDb,
				})
			}
		}
		sort.Sort(byModTimeDesc(listData.ListItem))
		err := templates.ExecuteTemplate(w, "list.html", listData)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
		}
	}
}

func StartService() {
	initTemplates()
	initValidPath()
	http.HandleFunc("/", listHandler)
	http.HandleFunc("/list/", listHandler)
	http.HandleFunc("/view/", makeHandler(viewHandler))
	http.HandleFunc("/edit/", makeHandler(editHandler))
	http.HandleFunc("/save/", makeHandler(saveHandler))
	addr := "0.0.0.0:20182"
	log.Printf("Listening %v for admin service...", addr)
	http.ListenAndServe(addr, nil)
}
