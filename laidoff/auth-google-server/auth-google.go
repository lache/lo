package main

import (
	"crypto/rsa"
	"log"
	"encoding/json"
	"encoding/pem"
	"crypto/x509"
	"net/http"
	"github.com/gasbank/laidoff/auth-google-server/jws"
	"os"
	"fmt"
	"io/ioutil"
	"strings"
	"github.com/gasbank/laidoff/match-server/convert"
	"github.com/gasbank/laidoff/db-server/user"
	"github.com/gasbank/laidoff/db-server/dbservice"
	"time"
)

const (
	ServiceName   = "auth-google"
	ServiceAddr   = ":10622"
	CertFile      = "key/popsongremix_com.cer"
	KeyFile       = "key/popsongremix_com.pem"
	WebClientId   = "933754889415-fr8buam0m8dh9hv71r2fb4au548lij0f.apps.googleusercontent.com"
	DbServiceAddr = ":20181"
)

var db dbservice.Db

func main() {
	// Set default log format
	log.SetFlags(log.LstdFlags | log.Lshortfile)
	log.SetOutput(os.Stdout)
	log.Printf("Greetings from %v service", ServiceName)
	db = dbservice.New(DbServiceAddr)
	http.HandleFunc("/auth-google", handleAuthGoogle)
	err := http.ListenAndServeTLS(ServiceAddr, CertFile, KeyFile, nil)
	if err != nil {
		log.Fatal("ListenAndServe: ", err)
	}
}

type AuthRequestBody struct {
	V1      int    `json:"v1"`
	V2      int    `json:"v2"`
	V3      int    `json:"v3"`
	V4      int    `json:"v4"`
	IdToken string `json:"idToken"`
}

func handleAuthGoogle(writer http.ResponseWriter, request *http.Request) {
	defer request.Body.Close()
	body, err := ioutil.ReadAll(request.Body)
	if err != nil {
		panic(err)
	}
	bodyStr := string(body)
	dec := json.NewDecoder(strings.NewReader(bodyStr))
	var authRequestBody AuthRequestBody
	if err := dec.Decode(&authRequestBody); err == nil {
		userId := convert.IdIntToByteArray([4]int{authRequestBody.V1, authRequestBody.V2, authRequestBody.V3, authRequestBody.V4})
		log.Printf("User ID: %v", user.IdByteArrayToString(userId))
		if ok := validate(authRequestBody.IdToken, userId); ok {
			writer.Write([]byte("validate ok"))
		} else {
			writer.Write([]byte("validate error"))
		}
	} else if err != nil {
		log.Printf("body parse error")
		writer.Write([]byte("body parse error"))
	}
}

func validate(idToken string, userId user.Id) bool {
	res, err := http.Get("https://www.googleapis.com/oauth2/v1/certs")
	if err != nil {
		log.Fatal(err)
		return false
	}
	defer res.Body.Close()
	var certs map[string]string
	dec := json.NewDecoder(res.Body)
	dec.Decode(&certs)
	// add error checking
	for googleCertKey, googleCert := range certs {
		blockPub, _ := pem.Decode([]byte(googleCert))
		certInterface, err := x509.ParseCertificates(blockPub.Bytes)
		pubKey := certInterface[0].PublicKey.(*rsa.PublicKey)
		log.Printf("Verifying token with cert key %v...", googleCertKey)
		err = jws.Verify(idToken, pubKey)
		if err != nil {
			log.Printf("Verify error: %v", err.Error())
		} else {
			log.Printf("Correctly signed token. Now checking embedded fields...")
			claimSet, err := jws.Decode(idToken)
			if err != nil {
				log.Printf("Decode error: %v", err.Error())
			} else {
				issuedAt:= time.Unix(claimSet.Iat, 0)
				expiredAt := time.Unix(claimSet.Exp, 0)
				now := time.Now()
				if now.Sub(issuedAt) < 0 {
					log.Printf("NOT VALID: ist is in future!")
				} else if expiredAt.Sub(now) < 0 {
					log.Printf("NOT VALID: already expired!")
				} else if claimSet.Aud == WebClientId {
					log.Printf("VALID: Good")
					claimSetStrBytes, err := json.MarshalIndent(claimSet, "", "  ")
					if err != nil {
						fmt.Println("error:", err)
					}
					log.Printf(string(claimSetStrBytes))
					var leaseDb user.LeaseDb
					err = db.Lease(&userId, &leaseDb)
					if err != nil {
						log.Printf("Lease error: %v", err.Error())
					} else {
						if leaseDb.Db.Nickname != claimSet.Name {
							log.Printf("Nickname changed from '%v' to '%v'",
								leaseDb.Db.Nickname, claimSet.Name)
							leaseDb.Db.Nickname = claimSet.Name
						}
						var writeReply int
						err = db.Write(&leaseDb, &writeReply)
						if err != nil {
							log.Printf("Write error: %v", err.Error())
						} else {
							return true
						}
					}
				} else {
					log.Printf("NOT VALID: NOT OUR TOKEN. MAYBE A FORGED ONE")
				}
			}
			break
		}
	}
	return false
}
