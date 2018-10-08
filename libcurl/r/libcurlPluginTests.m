opensslPluginTests ; OSE/SMH - Libcurl Tests;Oct 08, 2018@15:19
 ; (c) Sam Habiel 2018
 ; Licensed under Apache 2.0
 ;
TEST I $T(^%ut)="" QUIT
 D EN^%ut($t(+0),3)
 QUIT
 ;
T1 ; @TEST curl GET https://example.com
 d &libcurl.curl(.zzz,"GET","https://example.com")
 d CHKTF^%ut(zzz["Example Domain")
 quit
 ;
T2 ; @TEST curl GET https://rxnav.nlm.nih.gov/REST/rxcui/351772/allndcs.json
 d &libcurl.curl(.zzz,"GET","https://rxnav.nlm.nih.gov/REST/rxcui/351772/allndcs.json")
 d CHKTF^%ut(zzz["ndcConcept")
 quit
 ;
T3 ; @TEST curl GET https://rxnav.nlm.nih.gov/REST/rxcui/174742/related.json?rela=tradename_of+has_precise_ingredient
 d &libcurl.curl(.zzz,"GET","https://rxnav.nlm.nih.gov/REST/rxcui/174742/related.json?rela=tradename_of+has_precise_ingredient")
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
 d &libcurl.curlDo(.zzz,"GET","https://example.com")
 d CHKTF^%ut(zzz["Example Domain")
 d &libcurl.curlCleanup
 quit
 ;
TMI ; @TEST Multiple GETs from Single Domain - Init
 d &libcurl.curlInit
 quit
TM1 ; @TEST Multiple GETs from Single Domain - First
 d &libcurl.curlDo(.zzz,"GET","https://rxnav.nlm.nih.gov/REST/ndcstatus.json?ndc=00143314501")
 quit
TM2 ; @TEST Multiple GETs from Single Domain - Second
 d &libcurl.curlDo(.zzz,"GET","https://rxnav.nlm.nih.gov/REST/drugs?name=cymbalta")
 quit
TM3 ; @TEST Multiple GETs from Single Domain - Third
 d &libcurl.curlDo(.zzz,"GET","https://rxnav.nlm.nih.gov/REST/termtypes")
 quit
TM4 ; @TEST Multiple GETs from Single Domain - Fourth
 d &libcurl.curlDo(.zzz,"GET","https://rxnav.nlm.nih.gov/REST/brands?ingredientids=8896+20610")
 quit
TM5 ; @TEST Multiple GETs from Single Domain - Fifth
 d &libcurl.curlDo(.zzz,"GET","https://rxnav.nlm.nih.gov/REST/brands?ingredientids=8896+20610")
 quit
TM6 ; @TEST Mulitple GETs from Single Domain - Sixth
 d &libcurl.curlDo(.zzz,"GET","https://rxnav.nlm.nih.gov/REST/approximateTerm?term=zocor%2010%20mg&maxEntries=4")
 quit
TMC ; @TEST Multiple GETs from Single Domain - Cleanup
 d &libcurl.curlCleanup
 quit
