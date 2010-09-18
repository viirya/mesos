#! /usr/bin/env sh 

#cat <<EOF | qsub -l nodes=$1 -N $2
#date > /root/$2
#/root/hadoop-0.20.2/bin/hadoop dfs -put /root/$2-xhpl.out $2-xhpl.out
#EOF

#submit_job() {
#cat <<EOF | qsub -l nodes=$1 -N $2
#
#JOB_NAME=$PBS_JOBID
#(echo "PBS_O_WORKDIR is: $PBS_O_WORKDIR"
#date
#cd /nfs/hpl/$1node/ 
#mpiexec -n 24 /nfs/hpl/24node/xhpl
#date) > /root/$JOB_NAME-xhpl.out
#/root/hadoop-0.20.2/bin/hadoop dfs -put /root/$JOB_NAME-xhpl.out $JOB_NAME-xhpl.out
#EOF
#}

#submit_job 24 24_node.1

echo submitting 1st 12 node job
qsub -N 12_node.1 ./hpl-12node.qsub
sleep 3

echo submitting first 24 node job
qsub -N 24_node.1 ./hpl-24node.qsub
sleep 5

#echo submitting 2nd 24 node job
#qsub -N 24_node.2 ./hpl-12node.qsub
#sleep 5

echo submitting 1st 8 node job
qsub -N 8_node.1 ./hpl-8node.qsub
sleep $((60*10))

echo submitting 2nd 8 node job
qsub -N 8_node.2 ./hpl-8node.qsub
sleep $((60*2))

echo submitting 3rd 8 node job
qsub -N 8_node.3 ./hpl-8node.qsub
sleep 10 

echo submitting 2nd 12 node job
qsub -N 12_node.2 ./hpl-12node.qsub
sleep 3

echo submitting 4th 8 node job
qsub -N 8_node.4 ./hpl-8node.qsub

echo submitting 5th 8 node job
qsub -N 8_node.5 ./hpl-8node.qsub
