
sudo rmmod message_slot
sudo rm /dev/msgslot1
echo 'removed old'

make
echo 'built message_slot'
sudo insmod message_slot.ko
echo 'added kernal'
sudo mknod /dev/msgslot1 c 8 1
sudo chmod o+rw /dev/msgslot1
echo 'made file'

