#!/bin/bash
(
	echo "...TEST 1..."
	for (( i=0; i<50; i++ ))
	do
		./client.out "client"$((i+1)) 1 1>>testout.log &
	done &&

	wait

	echo "--TEST 1 COMPLETATO--"
	echo "...TEST 2..."

	for (( i=0; i<30; i++ ))
	do 
		./client.out "client"$((i+1)) 2 1>>testout.log &
	done

	echo "--TEST 2 COMPLETATO--"
	echo "...TEST 3..."

	for (( i=30; i<50; i++ ))
	do 
		./client.out "client"$((i+1)) 3 1>>testout.log &
	done

	wait
	echo "--TEST 3 COMPLETATO--"
)