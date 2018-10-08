opensslPluginTests ; OSE/SMH - Libcurl Tests;Oct 08, 2018@13:41
 ; (c) Sam Habiel 2018
 ; Licensed under Apache 2.0
 ;
TEST I $T(^%ut)="" QUIT
 D EN^%ut($t(+0),3)
 QUIT
 ;
T1 ; @TEST GET https://example.com"
 d &libcurl.curl(.zzz,"GET","https://example.com")
 d CHKTF^%ut(zzz["Example Domain")
 quit
 ;
T2 ; @TEST GET https://rxnav.nlm.nih.gov/REST/rxcui/351772/allndcs.json
 d &libcurl.curl(.zzz,"GET","https://rxnav.nlm.nih.gov/REST/rxcui/351772/allndcs.json")
 d CHKTF^%ut(zzz["ndcConcept")
 quit
 ;
T3 ; @TEST GET https://rxnav.nlm.nih.gov/REST/rxcui/174742/related.json?rela=tradename_of+has_precise_ingredient
 d &libcurl.curl(.zzz,"GET","https://rxnav.nlm.nih.gov/REST/rxcui/174742/related.json?rela=tradename_of+has_precise_ingredient")
 d CHKTF^%ut(zzz["relatedGroup")
 quit

 
