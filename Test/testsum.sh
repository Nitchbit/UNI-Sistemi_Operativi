#!/bin/bash

op_eseguite=0;
op_completate=0;
op_fallite=0;
client_lanciati=0;
client_anomali=0;
anomalie_test1=0;
anomalie_test2=0;
anomalie_test3=0;

test=0;

while read line; do
	value="$line"
	str="$(cut -d' ' -f1 <<< "$value")"
   	if [ ${str} = "Nome" ]; then
   		let "client_lanciati++"
   		continue
  	fi
    if [ ${str} = "Tipo" ]; then
    	str="$(cut -d' ' -f3 <<< "$value")"
    	test="$str"
    	continue
    fi
    str="$(cut -d' ' -f2 <<< "$value")"
   	if [ ${str} = "eseguite:" ]; then
    	str="$(cut -d' ' -f3 <<< "$value")"
   		let "op_eseguite+="$str""
   		continue
   	fi
   	if [ ${str} = "completate:" ]; then 
  		str="$(cut -d' ' -f3 <<< "$value")"
   		let "op_completate+="$str""
   		continue
    fi
   	if [ ${str} = "fallite:" ]; then
    	str="$(cut -d' ' -f3 <<< "$value")"
   		let "op_fallite+="$str""
    	if [ ${str} != "0" ]; then
    		let "client_anomali++"
    		if [ ${test} = "1" ]; then
    			let "anomalie_test1+="$str""
    			continue
    		fi
    		if [ ${test} = "2" ]; then 
    			let "anomalie_test2+="$str""
    			continue
  			fi
    		if [ ${test} = "3" ]; then
  				let "anomalie_test3+="$str""
  				continue
     		fi
   		fi
  	fi
done < "$1"

echo "Client lanciati: $client_lanciati"
echo "Client anomali: $client_anomali"
echo "Operazioni eseguite: $op_eseguite"
echo "Operazioni completate: $op_completate"
echo "Operazioni fallite: $op_fallite"
echo "Anomalie test 1: $anomalie_test1"
echo "Anomalie test 2: $anomalie_test2"
echo "Anomalie test 3: $anomalie_test3"

pidserver="$(pidof server.out)"

echo "...Invio SIGUSR1 al server..."

kill -SIGUSR1 $pidserver

echo "...Invio SIGTERM al server..."

kill -SIGTERM $pidserver