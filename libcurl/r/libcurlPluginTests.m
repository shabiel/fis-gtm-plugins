opensslPluginTests ; OSE/SMH - Libcurl Tests;Oct 08, 2018@13:32
 ; (c) Sam Habiel 2018
 ; Licensed under Apache 2.0
 ;
TEST I $T(^%ut)="" QUIT
 D EN^%ut($t(+0),3)
 QUIT
 ;
T1 ; @TEST
 d &libcurl.curl(.zzz,"GET","https://example.com")
 d CHKTF^%ut(zzz["Example Domain")
 quit
 ;
T2 ; @TEST
 d &libcurl.curl(.zzz,"GET","https://rxnav.nlm.nih.gov/REST/rxcui/351772/allndcs.json")
 d CHKTF^%ut(zzz["ndcConcept")
 quit
 
