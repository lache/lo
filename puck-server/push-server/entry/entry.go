package entry

import (
	"encoding/gob"
	"errors"
	"fmt"
	"github.com/NaySoftware/go-fcm"
	"github.com/sideshow/apns2"
	"github.com/sideshow/apns2/certificate"
	"html/template"
	"io"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"net/rpc"
	"os"
	"puck-server/db-server/user"
	"puck-server/shared-server"
	"regexp"
)

type UserId [16]byte

var templates *template.Template
var validPushTokenPath *regexp.Regexp

const (
	ANDROIDKEYPATH     = "cert/dev/fcmserverkey"
	APPLEKEYPATH       = "cert/dev/cert.p12"
	ANDROIDPRODKEYPATH = "cert/prod/fcmserverkey"
	APPLEPRODKEYPATH   = "cert/prod/cert.p12"
)

func initTemplates() {
	templates = template.Must(template.ParseFiles("editPushToken.html", "edit.html", "view.html", "push.html", "writePush.html", "certs.html"))
	validPushTokenPath = regexp.MustCompile("^/(savePushToken|editPushToken|deletePushToken|sendTestPush|writePush|sendPush)/([a-zA-Z0-9-:_]+)$")
}

func registerArith(server *rpc.Server, arith shared_server.Arith, pushService shared_server.PushService) {
	// registers Arith interface by name of `Arithmetic`.
	// If you want this name to be same as the type name, you
	// can use server.Register instead.
	server.RegisterName("Arithmetic", arith)
	server.RegisterName("PushService", pushService)
}

type Arith int

func (t *Arith) Multiply(args *shared_server.Args, reply *int) error {
	*reply = args.A * args.B
	return nil
}

func (t *Arith) Divide(args *shared_server.Args, quo *shared_server.Quotient) error {
	if args.B == 0 {
		return errors.New("divide by zero")
	}
	quo.Quo = args.A / args.B
	quo.Rem = args.A % args.B
	return nil
}

type PushUserData struct {
	UserId    [16]byte
	Domain    int
	Memo      string
	PushToken string
}

type Cert struct {
	FcmServerKey       string // Firebase Cloud Messaging server key
	FcmServerKeyPath   string // Firebase Cloud Messaging server key file path
	AppleServerKeyPath string // Apple Push Cert key path
}

type PushServiceData struct {
	PushTokenMap map[string]PushUserData // Push Token --> Push User Data
	UserIdMap    map[UserId]string       // User ID --> Push Token
	DevCert      Cert
	ProdCert     Cert
	Prod         bool // true if Production environment, false if not
}

func (pushServiceData *PushServiceData) Add(id user.Id, pushToken string, domain int) {
	var idFixed [16]byte
	copy(idFixed[:], id[:])
	pushServiceData.PushTokenMap[pushToken] = PushUserData{idFixed, domain, "", pushToken}
	pushServiceData.UserIdMap[idFixed] = pushToken
}

func (pushServiceData *PushServiceData) DeletePushToken(pushToken string) {
	pushUserData := pushServiceData.PushTokenMap[pushToken]
	delete(pushServiceData.PushTokenMap, pushToken)
	delete(pushServiceData.UserIdMap, pushUserData.UserId)
}

type PushService struct {
	data PushServiceData
}

func (service *PushService) RegisterPushToken(args *shared_server.PushToken, reply *int) error {
	log.Printf("Register push token: %v, %v", args.Domain, args.PushToken)
	service.data.Add(args.UserId, args.PushToken, args.Domain)
	log.Printf("pushTokenMap len: %v", len(service.data.PushTokenMap))
	writePushServiceData(&service.data)
	//if args.Domain == 2 { // Android (FCM)
	//	PostAndroidMessage(t.FcmServerKey, args.PushToken)
	//} else if args.Domain == 1 {
	//	PostIosMessage(args.PushToken)
	//}
	*reply = 1
	return nil
}

func (service *PushService) Broadcast(args *shared_server.BroadcastPush, reply *int) error {
	log.Printf("Broadcast: %v", args)
	log.Printf("Broadcasting message to the entire push pool (len=%v)", len(service.data.PushTokenMap))
	for pushToken, pushUserData := range service.data.PushTokenMap {
		domain := pushUserData.Domain
		if domain == 2 {
			go PostAndroidMessage(service.data.DevCert.FcmServerKey, pushToken, args.Title, args.Body)
		} else if domain == 1 {
			go PostIosMessage(pushToken, args.Body, service.data.Prod)
		} else {
			log.Printf("Unknown domain: %v", domain)
			*reply = 0
			return nil
		}
	}
	*reply = 1
	return nil
}

func (pushServiceData *PushServiceData) loadAllCerts() {
	pushServiceData.loadAllFcmCerts()
	pushServiceData.loadAllAppleCerts()
}

func (pushServiceData *PushServiceData) loadAllFcmCerts() {
	pushServiceData.DevCert.loadFcmCert(ANDROIDKEYPATH)
	pushServiceData.ProdCert.loadFcmCert(ANDROIDPRODKEYPATH)
}

func (cert *Cert) loadFcmCert(keyPath string) {
	// Load Firebase Cloud Messaging (FCM) server key
	fcmServerKeyBuf, err := ioutil.ReadFile(keyPath)
	if err != nil {
		log.Printf("FCM server key load failed: %v", err.Error())
		cert.FcmServerKey = ""
		cert.FcmServerKeyPath = ""
	} else {
		cert.FcmServerKey = string(fcmServerKeyBuf)
		cert.FcmServerKeyPath = keyPath
	}
}

func (pushServiceData *PushServiceData) loadAllAppleCerts() {
	pushServiceData.DevCert.loadAppleCert(APPLEKEYPATH)
	pushServiceData.ProdCert.loadAppleCert(APPLEPRODKEYPATH)
}

func (cert *Cert) loadAppleCert(keyPath string) {
	// Check Apple push key cert
	_, err := ioutil.ReadFile(keyPath)
	if err != nil {
		log.Printf("Apple server key load failed: %v", err.Error())
		cert.AppleServerKeyPath = ""
	} else {
		cert.AppleServerKeyPath = keyPath
	}
}

func writePushServiceData(pushServiceData *PushServiceData) {
	pushKeyDbFile, err := os.Create("db/pushkeydb")
	if err != nil {
		log.Fatalf("pushkeydb create failed: %v", err.Error())
	}
	encoder := gob.NewEncoder(pushKeyDbFile)
	encoder.Encode(pushServiceData)
	pushKeyDbFile.Close()
}

func PostAndroidMessage(fcmServerKey string, pushToken string, title string, body string) {
	data := map[string]string{
		"msg": "Hello world!",
		"sum": "HW",
	}
	c := fcm.NewFcmClient(fcmServerKey)
	ids := []string{
		pushToken,
	}
	//xds := []string{
	//	pushToken,
	//}
	c.NewFcmRegIdsMsg(ids, data)
	noti := &fcm.NotificationPayload{
		Title: title,
		Body:  body,
		Sound: "default",
		Icon:  "ic_stat_name",
	}
	c.SetNotificationPayload(noti)
	//c.AppendDevices(xds)
	status, err := c.Send()
	if err != nil {
		log.Printf("FCM send error: %v", err.Error())
	} else {
		status.PrintResults()
	}
}

type Page struct {
	Title string
	Body  []byte
}

func (p *Page) save() error {
	filename := "pages/" + p.Title + ".txt"
	return ioutil.WriteFile(filename, p.Body, 0600)
}

func pushHandler(w http.ResponseWriter, _ *http.Request, pushService *PushService) {
	err := templates.ExecuteTemplate(w, "push.html", pushService.data)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func certsHandler(w http.ResponseWriter, _ *http.Request, pushService *PushService) {
	err := templates.ExecuteTemplate(w, "certs.html", pushService.data)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func uploadCertsHandler(w http.ResponseWriter, r *http.Request, pushService *PushService) {
	switch r.Method {
	case "GET":
		http.Redirect(w, r, "/certs/", http.StatusFound)
	case "POST":
		os.MkdirAll("cert/dev", os.ModePerm)
		os.MkdirAll("cert/prod", os.ModePerm)
		pushService.data.DevCert.FcmServerKeyPath = ""
		pushService.data.DevCert.AppleServerKeyPath = ""
		pushService.data.ProdCert.FcmServerKeyPath = ""
		pushService.data.ProdCert.AppleServerKeyPath = ""
		err := r.ParseMultipartForm(16 * 1024)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		m := r.MultipartForm
		files := m.File["file"]
		for i := range files {
			file, err := files[i].Open()
			defer file.Close()
			if err != nil {
				http.Error(w, err.Error(), http.StatusInternalServerError)
				return
			}
			log.Printf("Upload filename: %v", files[i].Filename)
			var dst *os.File
			switch files[i].Filename {
			case "dev-fcmserverkey":
				dst, err = os.Create(ANDROIDKEYPATH)
			case "dev-cert.p12":
				dst, err = os.Create(APPLEKEYPATH)
			case "prod-fcmserverkey":
				dst, err = os.Create(ANDROIDPRODKEYPATH)
			case "prod-cert.p12":
				dst, err = os.Create(APPLEPRODKEYPATH)
			}
			defer dst.Close()
			if err != nil {
				http.Error(w, err.Error(), http.StatusInternalServerError)
				return
			}
			if _, err := io.Copy(dst, file); err != nil {
				http.Error(w, err.Error(), http.StatusInternalServerError)
				return
			}
			switch files[i].Filename {
			case "dev-fcmserverkey":
				pushService.data.DevCert.FcmServerKeyPath = dst.Name()
			case "dev-cert.p12":
				pushService.data.DevCert.AppleServerKeyPath = dst.Name()
			case "prod-fcmserverkey":
				pushService.data.ProdCert.FcmServerKeyPath = dst.Name()
			case "prod-cert.p12":
				pushService.data.ProdCert.AppleServerKeyPath = dst.Name()
			}
		}
		pushService.data.loadAllCerts()
		http.Redirect(w, r, "/certs/", http.StatusFound)
	default:
		w.WriteHeader(http.StatusMethodNotAllowed)
	}
}

func setEnvHandler(w http.ResponseWriter, r *http.Request, pushService *PushService) {
	switch r.Method {
	case "GET":
		http.Redirect(w, r, "/certs/", http.StatusFound)
	case "POST":
		switch r.FormValue("env") {
		case "prod":
			pushService.data.Prod = true
		case "dev":
			pushService.data.Prod = false
		}
		http.Redirect(w, r, "/certs/", http.StatusFound)
	default:
		w.WriteHeader(http.StatusMethodNotAllowed)
	}
}

func deletePushTokenHandler(w http.ResponseWriter, r *http.Request, pushService *PushService, pushToken string) {
	pushService.data.DeletePushToken(pushToken)
	writePushServiceData(&pushService.data)
	http.Redirect(w, r, "/push/", http.StatusFound)
}

func sendTestPushHandler(w http.ResponseWriter, r *http.Request, pushService *PushService, pushToken string) {
	domain := pushService.data.PushTokenMap[pushToken].Domain
	if domain == 2 {
		PostAndroidMessage(pushService.data.DevCert.FcmServerKey, pushToken, "용사는 실직중", "어서와.. 테스트 푸시는 처음이지...?")
	} else if domain == 1 {
		PostIosMessage(pushToken, "어서와... 테스트 푸시는 처음이지?", false)
	} else {
		http.NotFound(w, r)
		return
	}
	http.Redirect(w, r, "/push/", http.StatusFound)
}

func savePushTokenHandler(w http.ResponseWriter, r *http.Request, pushService *PushService, pushToken string) {
	pushUserData := pushService.data.PushTokenMap[pushToken]
	memo := r.FormValue("memo")
	pushUserData.Memo = memo
	pushService.data.PushTokenMap[pushToken] = pushUserData
	writePushServiceData(&pushService.data)
	http.Redirect(w, r, "/push/", http.StatusFound)
}

func sendPushHandler(w http.ResponseWriter, r *http.Request, pushService *PushService, pushToken string) {
	body := r.FormValue("body")
	domain := pushService.data.PushTokenMap[pushToken].Domain
	if domain == 2 {
		PostAndroidMessage(pushService.data.DevCert.FcmServerKey, pushToken, "용사는 실직중", body)
	} else if domain == 1 {
		PostIosMessage(pushToken, body, pushService.data.Prod)
	} else {
		http.NotFound(w, r)
		return
	}
	http.Redirect(w, r, "/push/", http.StatusFound)
}

func editPushTokenHandler(w http.ResponseWriter, _ *http.Request, pushService *PushService, pushToken string) {
	pushUserData := pushService.data.PushTokenMap[pushToken]
	err := templates.ExecuteTemplate(w, "editPushToken.html", pushUserData)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func writePushHandler(w http.ResponseWriter, _ *http.Request, _ *PushService, pushToken string) {
	type WritePushData struct {
		PushToken string
	}
	err := templates.ExecuteTemplate(w, "writePush.html", WritePushData{pushToken})
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func makePushHandler(fn func(http.ResponseWriter, *http.Request, *PushService), pushServiceData *PushService) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		fn(w, r, pushServiceData)
	}
}

func makePushTokenHandler(fn func(http.ResponseWriter, *http.Request, *PushService, string), pushService *PushService) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		m := validPushTokenPath.FindStringSubmatch(r.URL.Path)
		if m == nil {
			http.NotFound(w, r)
			return
		}
		fn(w, r, pushService, m[2])
	}
}

func startAdminService(pushService *PushService) {
	http.HandleFunc("/", makePushHandler(pushHandler, pushService))
	http.HandleFunc("/push/", makePushHandler(pushHandler, pushService))
	http.HandleFunc("/deletePushToken/", makePushTokenHandler(deletePushTokenHandler, pushService))
	http.HandleFunc("/sendTestPush/", makePushTokenHandler(sendTestPushHandler, pushService))
	http.HandleFunc("/writePush/", makePushTokenHandler(writePushHandler, pushService))
	http.HandleFunc("/sendPush/", makePushTokenHandler(sendPushHandler, pushService))
	http.HandleFunc("/editPushToken/", makePushTokenHandler(editPushTokenHandler, pushService))
	http.HandleFunc("/savePushToken/", makePushTokenHandler(savePushTokenHandler, pushService))
	http.HandleFunc("/certs/", makePushHandler(certsHandler, pushService))
	http.HandleFunc("/uploadCerts/", makePushHandler(uploadCertsHandler, pushService))
	http.HandleFunc("/setEnv/", makePushHandler(setEnvHandler, pushService))
	addr := "0.0.0.0:18080"
	log.Printf("Listening %v for admin service...", addr)
	http.ListenAndServe(addr, nil)
}

func Entry() {
	// Set default log format
	log.SetFlags(log.LstdFlags | log.Lshortfile)
	log.SetOutput(os.Stdout)
	log.Println("Greetings from push server")
	initTemplates()
	// Create db directory to save user database
	os.MkdirAll("db", os.ModePerm)
	os.MkdirAll("pages", os.ModePerm)
	pushService := &PushService{
		data: PushServiceData{
			PushTokenMap: make(map[string]PushUserData),
			UserIdMap:    make(map[UserId]string),
		},
	}
	if len(os.Args) >= 2 && os.Args[1] == "prod" {
		log.Println("PRODUCTION MODE")
		pushService.data.Prod = true
	} else {
		pushService.data.Prod = false
	}
	pushService.data.loadAllCerts()
	pushKeyDbFile, err := os.Open("db/pushkeydb")
	if err != nil {
		if os.IsNotExist(err) {
			log.Printf("Push key DB not found.")
		} else {
			log.Fatalf("pushkeydb create failed: %v", err.Error())
		}
	} else {
		defer pushKeyDbFile.Close()
		decoder := gob.NewDecoder(pushKeyDbFile)
		err := decoder.Decode(&pushService.data)
		if err != nil {
			log.Fatalf("pushkeydb decode failed: %v", err.Error())
		}
		log.Printf("pushTokenMap loaded from file. len: %v", len(pushService.data.PushTokenMap))
	}
	go startAdminService(pushService)

	// Register a new rpc server (In most cases, you will use default server only)
	// And register struct we created above by name "Arith"
	// The wrapper method here ensures that only structs which implement Arith interface
	// are allowed to register themselves.
	server := rpc.NewServer()
	//Creating an instance of struct which implement Arith interface
	arith := new(Arith)
	registerArith(server, arith, pushService)

	addr := ":20171"
	log.Printf("Listening %v for push service...", addr)
	// Listen for incoming tcp packets on specified port.
	l, e := net.Listen("tcp", addr)
	if e != nil {
		log.Fatal("listen error:", e)
	}

	// This statement links rpc server to the socket, and allows rpc server to accept
	// rpc request coming from that socket.
	server.Accept(l)
}

func PostIosMessage(pushToken string, body string, prod bool) {
	var appleKeyPath string
	if prod {
		appleKeyPath = APPLEPRODKEYPATH
	} else {
		appleKeyPath = APPLEKEYPATH
	}
	cert, err := certificate.FromP12File(appleKeyPath, "")
	if err != nil {
		log.Fatalf("Apple Push Cert Error: %v", err)
	}

	notification := &apns2.Notification{}
	notification.DeviceToken = pushToken
	notification.Topic = "com.popsongremix.laidoff"
	notification.Payload = []byte(fmt.Sprintf(`{"aps":{"alert":"%s"}}`, body))

	var client *apns2.Client
	if prod {
		client = apns2.NewClient(cert).Production()
	} else {
		client = apns2.NewClient(cert).Development()
	}
	res, err := client.Push(notification)

	if err != nil {
		log.Fatal("Error:", err)
	}

	log.Printf("Ios push (token %v) result: %v %v %v", pushToken, res.StatusCode, res.ApnsID, res.Reason)
}
