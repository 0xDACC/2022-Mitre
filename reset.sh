python3 tools/run_saffire.py kill-system --emulated --sysname 0xDACC-test

docker image rm 0xDACC-test/host_tools:latest

docker image rm 0xDACC-test/bootloader:latest

docker volume prune

docker container prune

rm -rf socks
mkdir socks

chmod ugo+rwx host_tools/*

echo ""
echo ""
echo ""
echo "............................."
echo "....|Emulation Updated|......"
echo "............................."