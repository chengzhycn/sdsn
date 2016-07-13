tc qdisc del dev eth1 root
tc qdisc add dev eth1 root handle 1: prio bands 16
tc filter add dev eth1 parent 1: protocol ip prio 16 u32 match ip src 10.0.50.49/32 match ip dst 10.0.50.48/32 flowid 1:1
tc qdisc add dev eth1 parent 1:1 handle 10: tbf rate 80Kbit buffer 16000 limit 32000
sleep 22
echo 1
tc qdisc add dev eth1 parent 10: handle 100: netem loss 1.5%
sleep 22
echo 2
tc qdisc change dev eth1 parent 10: handle 100: netem loss 2%
sleep 22
echo 3
tc qdisc change dev eth1 parent 10: handle 100: netem loss 2.5%
sleep 22
echo 4
tc qdisc change dev eth1 parent 10: handle 100: netem loss 3%
sleep 22
echo 5
tc qdisc change dev eth1 parent 10: handle 100: netem loss 3.5%
sleep 22
echo 6
tc qdisc change dev eth1 parent 10: handle 100: netem loss 4%
sleep 22
echo 7
tc qdisc change dev eth1 parent 10: handle 100: netem loss 4.5%
sleep 22
echo 8
tc qdisc change dev eth1 parent 10: handle 100: netem loss 5%
sleep 22
echo end

