#! /bin/sh
#submit multiple jobs

div ()  # Arguments: dividend and divisor
{
        if [ $2 -eq 0 ]; then echo division by 0; exit; fi
        local p=12                            # precision
        local c=${c:-0}                       # precision counter
        local d=.                             # decimal separator
        local r=$(($1/$2)); echo -n $r        # result of division
        local m=$(($r*$2))
        [ $c -eq 0 ] && [ $m -ne $1 ] && echo -n $d
        [ $1 -eq $m ] || [ $c -eq $p ] && return
        local e=$(($1-$m))
        let c=c+1
        div $(($e*10)) $2
}  

# rm if already exist
[ -e out.txt ] && rm out.txt
# write the headline
echo "SJF_wait_time, SRT_wait_time, alpha, lambda, " >> out.txt

for lambda in {0.01,0.001} 
	do
		for i in {1..99}
			do
				alpha=$(div $i 100)                  # write to variabl                  # write to variable
    			./main 2 $lambda 200 5 4 $alpha 120 
    			head -10 simout.txt | grep -o 'wait time: [0-9]*.[0-9]* ' | tr -dc '0-9*.0-9* ' >> out.txt
    			echo $alpha	 $lambda >> out.txt
    			
		done
	done
