opensslPluginTests ; OSE/SMH - Libcurl Tests;2018-12-04  11:30 AM
 ; (c) Sam Habiel 2018,2023
 ; Licensed under Apache 2.0
 ;
TEST I $T(^%ut)="" QUIT
 D EN^%ut($t(+0),3)
 QUIT
 ;
T1 ; @TEST curl GET https://example.com
 n sss,zzz
 n status s status=$&libcurl.curl(.sss,.zzz,"GET","https://example.com")
 d CHKEQ^%ut(status,0)
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut(zzz["Example Domain")
 quit
 ;
T2 ; @TEST curl GET https://rxnav.nlm.nih.gov/REST/rxcui/351772/allndcs.json
 n sss,zzz
 n status s status=$&libcurl.curl(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/rxcui/351772/allndcs.json")
 d CHKEQ^%ut(status,0)
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut(zzz["ndcConcept")
 quit
 ;
T3 ; @TEST curl GET https://rxnav.nlm.nih.gov/REST/rxcui/174742/related.json?rela=tradename_of+has_precise_ingredient
 n sss,zzz
 n status s status=$&libcurl.curl(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/rxcui/174742/related.json?rela=tradename_of+has_precise_ingredient")
 d CHKEQ^%ut(status,0)
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut(zzz["relatedGroup")
 quit
 ;
T4 ; @TEST init, cleanup runs w/o errors
 d &libcurl.init
 d &libcurl.cleanup
 quit
 ;
T5 ; @TEST do GET https://example.com
 n sss,zzz
 d &libcurl.init
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://example.com")
 d CHKEQ^%ut(status,0)
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut(zzz["Example Domain")
 d &libcurl.cleanup
 quit
 ;
TMI ; @TEST Multiple GETs from Single Domain - Init
 d &libcurl.init
 quit
TM1 ; @TEST Multiple GETs from Single Domain - First
 n sss,zzz
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/ndcstatus.json?ndc=00143314501")
 d CHKEQ^%ut(status,0)
 quit
TM2 ; @TEST Multiple GETs from Single Domain - Second
 n sss,zzz
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/drugs?name=cymbalta")
 d CHKEQ^%ut(status,0)
 quit
TM3 ; @TEST Multiple GETs from Single Domain - Third
 n sss,zzz
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/termtypes")
 d CHKEQ^%ut(status,0)
 quit
TM4 ; @TEST Multiple GETs from Single Domain - Fourth
 n sss,zzz
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/brands?ingredientids=8896+20610")
 d CHKEQ^%ut(status,0)
 quit
TM5 ; @TEST Multiple GETs from Single Domain - Fifth
 n sss,zzz
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/brands?ingredientids=8896+20610")
 d CHKEQ^%ut(status,0)
 quit
TM6 ; @TEST Mulitple GETs from Single Domain - Sixth
 n sss,zzz
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/approximateTerm?term=zocor%2010%20mg&maxEntries=4")
 d CHKEQ^%ut(status,0)
 quit
TMC ; @TEST Multiple GETs from Single Domain - Cleanup
 d &libcurl.cleanup
 quit
 ;
TPAY ; @TEST Test Payload
 n sss,zzz
 N PAYLOAD,RTN,H,RET
 N CRLF S CRLF=$C(13,10)
 N R S R=$R(123423421234)
 S PAYLOAD(1)="KBANTEST ; VEN/SMH - Test routine for Sam ;"_R
 S PAYLOAD(2)=" QUIT"
 S PAYLOAD=PAYLOAD(1)_CRLF_PAYLOAD(2)
 n status s status=$&libcurl.curl(.sss,.zzz,"POST","https://httpbin.org/post",PAYLOAD)
 d CHKEQ^%ut(status,0)
 d CHKEQ^%ut(sss,200)
 D CHKTF^%ut(zzz[R)
 QUIT
 ;
TPAY0 ; @TEST Test empty payload
 n sss,zzz
 N PAYLOAD,RTN,H,RET
 N CRLF S CRLF=$C(13,10)
 n status s status=$&libcurl.curl(.sss,.zzz,"POST","https://httpbin.org/post","")
 d CHKEQ^%ut(status,0)
 d CHKEQ^%ut(sss,200)
 QUIT
 ;
TPAYMIME ; @TEST Test Payload with mime type
 n sss,zzz
 N PAYLOAD,RTN,H,RET
 N CRLF S CRLF=$C(13,10)
 N R S R=$R(123423421234)
 S PAYLOAD="{""test"":"_R_"}"
 n status s status=$&libcurl.curl(.sss,.zzz,"POST","https://httpbin.org/post",PAYLOAD,"application/json")
 d CHKEQ^%ut(status,0)
 d CHKEQ^%ut(sss,200)
 D CHKTF^%ut(zzz[R)
 QUIT
 ;
TTO ; @TEST do GET https://example.com with full timeout in seconds
 n sss,zzz
 d &libcurl.init
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://example.com",,,5)
 d CHKEQ^%ut(status,0)
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut(zzz["Example Domain")
 d &libcurl.cleanup
 quit
 ;
TCTO ; @TEST do GET https://example.com with connect timeout in milliseconds
 n sss,zzz,status
 d &libcurl.init
 do
 . n $et s $et="set $ec="""""
 . d &libcurl.conTimeoutMS(1)
 . s status=$&libcurl.do(.sss,.zzz,"GET","https://example.com")
 d &libcurl.cleanup
 d CHKEQ^%ut($data(status),0)
 d CHKEQ^%ut(sss,-28)
 d CHKEQ^%ut(zzz,"Timeout was reached")
 quit
 ;
THGET ; @TEST do GET https://example.com with headers
 n sss,zzz,headers
 n crlf s crlf=$C(13,10)
 d &libcurl.init
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://example.com",,,5,.headers)
 d CHKEQ^%ut(status,0)
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut(zzz["Example Domain")
 d CHKTF^%ut(headers["etag:")
 d &libcurl.cleanup
 quit
 ;
THSEND ; @TEST do Send Custom Headers
 n sss,zzz,headers
 n crlf s crlf=$C(13,10)
 d &libcurl.init
 d &libcurl.addHeader("DNT: 1")
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://httpbin.org/headers",,,,.headers)
 d CHKEQ^%ut(status,0)
 d &libcurl.cleanup
 d CHKTF^%ut($ZCO(zzz,"U")["DNT")
 quit
TB100 ; @TEST curl 100 bytes of binary data
 n sss,zzz
 d &libcurl.init
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://httpbin.org/stream-bytes/100")
 d CHKEQ^%ut(status,0)
 d &libcurl.cleanup
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut($l(zzz)=100)
 quit
 ;
TB1M ; @TEST curl with >1M bytes of binary data
 n sss,zzz,ec
 d &libcurl.init
 n $et,$es
 s $et="s ec=$ec,$ec="""""
 n status
 d 
 . s status=$&libcurl.do(.sss,.zzz,"GET","https://gitlab.com/YottaDB/DB/YDB/uploads/d25f3a2cc5e0cc5fdf9482796976cda1/yottadb_r122_src.tgz")
 d CHKEQ^%ut(status,255)
 d &libcurl.cleanup
 quit
 ;
TBAUTH ; @TEST Basic authoriazation
 n sss,zzz,ec
 d &libcurl.init
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://httpbin.org/basic-auth/boo/foo",,"application/json")
 d &libcurl.cleanup
 d CHKEQ^%ut(sss,401)
 d &libcurl.init
 d &libcurl.auth("Basic","boo:foo")
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://httpbin.org/basic-auth/boo/foo",,"application/json")
 d &libcurl.cleanup
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut(zzz["boo")
 quit
 ;
TCERT1 ; @TEST Test TLS with a client certificate no key password
 W !!
 N %CMD
 S %CMD="openssl req -x509 -nodes -days 365 -sha256 -subj '/C=US/ST=Washington/L=Seattle/CN=www.smh101.com' -newkey rsa:2048 -keyout /tmp/mycert.key -out /tmp/mycert.pem"
 ZSY %CMD
 S %CMD="openssl req -new -nodes -newkey rsa:2048 -subj '/C=US/ST=Washington/L=Seattle/CN=www.smh101.com' -keyout /tmp/client.key -out /tmp/client.csr"
 ZSY %CMD
 S %CMD="openssl x509 -req -in /tmp/client.csr -CA /tmp/mycert.pem -CAkey /tmp/mycert.key -CAcreateserial -out /tmp/client.pem -days 1024 -sha256"
 ZSY %CMD
 ;
 n sss,zzz,ec
 d &libcurl.init
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://prod.idrix.eu/secure/")
 d CHKTF^%ut(zzz["No SSL client certificate presented")
 d &libcurl.cleanup
 d &libcurl.init
 d &libcurl.clientTLS("/tmp/client.pem","/tmp/client.key")
 n status s status=$&libcurl.do(.sss,.zzz,"GET","https://prod.idrix.eu/secure/")
 d CHKTF^%ut(zzz["SSL Authentication OK!")
 d &libcurl.cleanup
 zsy "rm /tmp/mycert* /tmp/client*"
 quit
 ;
TCERT2 ; @TEST Test TLS with a client certifiate with key password
 I $ZV["Darwin" QUIT  ; Darwin always asks for password; so I give up.
 N %CMD
 S %CMD="openssl genrsa -aes128 -passout pass:monkey1234 -out /tmp/mycert.key 2048"
 ZSY %CMD
 ;
 S %CMD="openssl req -new -key /tmp/mycert.key -passin pass:monkey1234 -subj '/C=US/ST=Washington/L=Seattle/CN=www.smh101.com' -out /tmp/mycert.csr"
 ZSY %CMD
 ;
 S %CMD="openssl req -x509 -days 365 -sha256 -in /tmp/mycert.csr -key /tmp/mycert.key -passin pass:monkey1234 -out /tmp/mycert.pem"
 ZSY %CMD
 ;
 n status
 n $es,$et s $et="q:$es>1  s $ec="""""
 d
 . d &libcurl.init
 . d &libcurl.clientTLS("/tmp/client.pem","/tmp/client.key")
 . s status=$&libcurl.do(.sss,.zzz,"GET","https://prod.idrix.eu/secure/")
 . d &libcurl.cleanup
 D CHKTF^%ut(status)
 n sss,zzz,ec
 d &libcurl.init
 d &libcurl.clientTLS("/tmp/client.pem","/tmp/client.key","monkey1234")
 s status=$&libcurl.do(.sss,.zzz,"GET","https://prod.idrix.eu/secure/")
 d CHKEQ^%ut(status,0)
 d CHKTF^%ut(zzz["SSL Authentication OK!")
 d &libcurl.cleanup
 ;
 zsy "rm /tmp/mycert* /tmp/client*"
 ;
 quit
