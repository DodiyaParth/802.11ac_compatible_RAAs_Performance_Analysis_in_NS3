1) Copy the file "802.11ac_raa_comparision.cc" into /scratch from Ns3 root directory.
2) Run ./waf --run "802.11ac_raa_comparision" to build and run the file.
3) Command line options, example, ./waf --run "802.11ac_raa_comparision -raa=Ideal"

**Relevant Options available**                                             
  nCsma                         ->                                      Number of CSMA nodes
  
  nWifi                           ->                                    Number of wifi STA devices
  
  raa                               ->                                  RAA algortihm ( MinstrelHt / ConstantRate / Ideal )
  
  maxBytes                            ->                                Max numbr of bytes to be sent for TCP Streams.
  
  
  For other options refer the main file.
  
4) Running the file will generate two output files,

   a) Delay_"raa_name"_"nWifi".csv -> Records Delay per second
   
   b) Throughput_"raa_name"_"nWifi".csv -> Records Throughput per second
   
   
5) IEEE Standard, Delay Model, Loss Model, Error Model can changed according in the file.








#For any query, contact,
-> Dhruv Agja - dhruvagja8808@gmail.com
-> Parth Dodiya - 
-> Avdesh Yadav - 
