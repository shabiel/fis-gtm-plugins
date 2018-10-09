opensslPluginTests ; OSE/SMH - Libcurl Tests;Oct 09, 2018@14:08
 ; (c) Sam Habiel 2018
 ; Licensed under Apache 2.0
 ;
TEST I $T(^%ut)="" QUIT
 D EN^%ut($t(+0),3)
 QUIT
 ;
T1 ; @TEST curl GET https://example.com
 d &libcurl.curl(.sss,.zzz,"GET","https://example.com")
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut(zzz["Example Domain")
 quit
 ;
T2 ; @TEST curl GET https://rxnav.nlm.nih.gov/REST/rxcui/351772/allndcs.json
 d &libcurl.curl(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/rxcui/351772/allndcs.json")
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut(zzz["ndcConcept")
 quit
 ;
T3 ; @TEST curl GET https://rxnav.nlm.nih.gov/REST/rxcui/174742/related.json?rela=tradename_of+has_precise_ingredient
 d &libcurl.curl(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/rxcui/174742/related.json?rela=tradename_of+has_precise_ingredient")
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut(zzz["relatedGroup")
 quit
 ;
T4 ; @TEST curlInit, curlCleanup runs w/o errors
 d &libcurl.curlInit
 d &libcurl.curlCleanup
 quit
 ;
T5 ; @TEST curlDo GET https://example.com
 d &libcurl.curlInit
 d &libcurl.curlDo(.sss,.zzz,"GET","https://example.com")
 d CHKEQ^%ut(sss,200)
 d CHKTF^%ut(zzz["Example Domain")
 d &libcurl.curlCleanup
 quit
 ;
TMI ; @TEST Multiple GETs from Single Domain - Init
 d &libcurl.curlInit
 quit
TM1 ; @TEST Multiple GETs from Single Domain - First
 d &libcurl.curlDo(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/ndcstatus.json?ndc=00143314501")
 quit
TM2 ; @TEST Multiple GETs from Single Domain - Second
 d &libcurl.curlDo(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/drugs?name=cymbalta")
 quit
TM3 ; @TEST Multiple GETs from Single Domain - Third
 d &libcurl.curlDo(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/termtypes")
 quit
TM4 ; @TEST Multiple GETs from Single Domain - Fourth
 d &libcurl.curlDo(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/brands?ingredientids=8896+20610")
 quit
TM5 ; @TEST Multiple GETs from Single Domain - Fifth
 d &libcurl.curlDo(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/brands?ingredientids=8896+20610")
 quit
TM6 ; @TEST Mulitple GETs from Single Domain - Sixth
 d &libcurl.curlDo(.sss,.zzz,"GET","https://rxnav.nlm.nih.gov/REST/approximateTerm?term=zocor%2010%20mg&maxEntries=4")
 quit
TMC ; @TEST Multiple GETs from Single Domain - Cleanup
 d &libcurl.curlCleanup
 quit
 ;
TPAY ; @TEST Test Payload
 N PAYLOAD,RTN,H,RET
 N CRLF S CRLF=$C(13,10)
 N R S R=$R(123423421234)
 S PAYLOAD(1)="KBANTEST ; VEN/SMH - Test routine for Sam ;"_R
 S PAYLOAD(2)=" QUIT"
 S PAYLOAD=PAYLOAD(1)_CRLF_PAYLOAD(2)
 d &libcurl.curl(.sss,.zzz,"POST","https://httpbin.org/post",PAYLOAD)
 d CHKEQ^%ut(sss,200)
 D CHKTF^%ut(zzz[R)
 QUIT
TPAYMIME ; @TEST Test Payload with mime type
 N PAYLOAD,RTN,H,RET
 N CRLF S CRLF=$C(13,10)
 N R S R=$R(123423421234)
 S PAYLOAD="{""test"":"_R_"}"
 d &libcurl.curl(.sss,.zzz,"POST","https://httpbin.org/post",PAYLOAD,"application/json")
 d CHKEQ^%ut(sss,200)
 D CHKTF^%ut(zzz[R)
 w zzz
 QUIT
