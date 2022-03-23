grep "avg_resp_time" result.txt | awk '{print $2 " "} ' | tr  '\n' ',' > out.txt
echo '' >> out.txt
grep "goodput" result.txt | awk '{print $2 " "} ' | tr  '\n' ',' >> out.txt
echo '' >> out.txt
grep "badput" result.txt | awk '{print $2 " "} ' | tr  '\n' ',' >> out.txt
echo '' >> out.txt
grep "throughput" result.txt | awk '{print $2 " "} ' | tr  '\n' ',' >> out.txt
echo '' >> out.txt
grep "avg_util" result.txt | awk '{print $2 " "} ' | tr  '\n' ',' >> out.txt
echo '' >> out.txt
grep "drops" result.txt | awk '{print $2 " "} ' | tr  '\n' ',' >> out.txt
