 
#!/bin/bash
 
ID=$1

loglist=`ls /home/ubuntu/results/$ID -1 | grep "log"`

echo "nvox_XY  nvox_Z   nChunks    userTime (minutes | seconds)    sysTime (minutes | seconds)     RMSE           PSNR           NRMSD          NMAD" &> results-$ID.dat

for log in $loglist; do

    nvox_XY=`echo "$log" | cut -d '-' -f 2`
    nvox_Z=`echo "$log" | cut -d '-' -f 3`
    nChunks=`echo "$log" | cut -d '-' -f 4`

    TIMEUSER=`grep "user" /home/ubuntu/results/$ID/$log | cut -d$'\t' -f2`
    TIMESYST=`grep "sys" /home/ubuntu/results/$ID/$log | cut -d$'\t' -f2`
    RMSE=`grep "RMSE" /home/ubuntu/results/$ID/$log | cut -d ' ' -f 4`
    PSNR=`grep "PSNR" /home/ubuntu/results/$ID/$log | cut -d ' ' -f 4`
    NRMSD=`grep "NRMSD" /home/ubuntu/results/$ID/$log | cut -d ' ' -f 3`
    NMAD=`grep "NMAD" /home/ubuntu/results/$ID/$log | cut -d ' ' -f 4`
    
    TIMEUSERMIN=`echo "$TIMEUSER" | cut -d 'm' -f1`
    TIMEUSERSEC=`echo "$TIMEUSER" | cut -d 'm' -f2`
    TIMEUSERSEC=`echo "$TIMEUSERSEC" | cut -d 's' -f1`

    TIMESYSTMIN=`echo "$TIMESYST" | cut -d 'm' -f1`
    TIMESYSTSEC=`echo "$TIMESYST" | cut -d 'm' -f2`
    TIMESYSTSEC=`echo "$TIMESYSTSEC" | cut -d 's' -f1`
    
    echo " $nvox_XY     $nvox_Z    $nChunks            $TIMEUSERMIN       $TIMEUSERSEC                $TIMESYSTMIN       $TIMESYSTSEC            $RMSE   $PSNR   $NRMSD   $NMAD" >> results-$ID.dat

done
