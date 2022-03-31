python3 tools/run_saffire.py kill-system --emulated --sysname dillon

python3 tools/run_saffire.py launch-bootloader --emulated  --sysname dillon --sock-root socks/ --uart-sock 1234

echo " ---------------"
echo "|RUNNING 1K TEST|
echo " ---------------

python3 tools/run_saffire.py fw-protect --emulated --sysname dillon --fw-root firmware/ --raw-fw-file 1k.bin --protected-fw-file example_fw.prot --fw-version 4 --fw-message 'version 4'


python3 tools/run_saffire.py kill-system --emulated --sysname dillon

python3 tools/run_saffire.py launch-bootloader --emulated  --sysname dillon --sock-root socks/ --uart-sock 1234

echo " ---------------"
echo "|RUNNING 2K TEST|
echo " ---------------

python3 tools/run_saffire.py fw-protect --emulated --sysname dillon --fw-root firmware/ --raw-fw-file 2k.bin --protected-fw-file example_fw.prot --fw-version 4 --fw-message 'version 4'


python3 tools/run_saffire.py kill-system --emulated --sysname dillon

python3 tools/run_saffire.py launch-bootloader --emulated  --sysname dillon --sock-root socks/ --uart-sock 1234

echo " ---------------"
echo "|RUNNING 3K TEST|
echo " ---------------

python3 tools/run_saffire.py fw-protect --emulated --sysname dillon --fw-root firmware/ --raw-fw-file 3k.bin --protected-fw-file example_fw.prot --fw-version 4 --fw-message 'version 4'


python3 tools/run_saffire.py kill-system --emulated --sysname dillon

python3 tools/run_saffire.py launch-bootloader --emulated  --sysname dillon --sock-root socks/ --uart-sock 1234

echo " ---------------"
echo "|RUNNING 4K TEST|
echo " ---------------

python3 tools/run_saffire.py fw-protect --emulated --sysname dillon --fw-root firmware/ --raw-fw-file 4k.bin --protected-fw-file example_fw.prot --fw-version 4 --fw-message 'version 4'


python3 tools/run_saffire.py kill-system --emulated --sysname dillon

python3 tools/run_saffire.py launch-bootloader --emulated  --sysname dillon --sock-root socks/ --uart-sock 1234

echo " ---------------"
echo "|RUNNING 5K TEST|
echo " ---------------

python3 tools/run_saffire.py fw-protect --emulated --sysname dillon --fw-root firmware/ --raw-fw-file 5k.bin --protected-fw-file example_fw.prot --fw-version 4 --fw-message 'version 4'


python3 tools/run_saffire.py kill-system --emulated --sysname dillon

python3 tools/run_saffire.py launch-bootloader --emulated  --sysname dillon --sock-root socks/ --uart-sock 1234

echo " ---------------"
echo "|RUNNING 6K TEST|
echo " ---------------

python3 tools/run_saffire.py fw-protect --emulated --sysname dillon --fw-root firmware/ --raw-fw-file 6k.bin --protected-fw-file example_fw.prot --fw-version 4 --fw-message 'version 4'


python3 tools/run_saffire.py kill-system --emulated --sysname dillon

python3 tools/run_saffire.py launch-bootloader --emulated  --sysname dillon --sock-root socks/ --uart-sock 1234

echo " ---------------"
echo "|RUNNING 7K TEST|
echo " ---------------

python3 tools/run_saffire.py fw-protect --emulated --sysname dillon --fw-root firmware/ --raw-fw-file 7k.bin --protected-fw-file example_fw.prot --fw-version 4 --fw-message 'version 4'


python3 tools/run_saffire.py kill-system --emulated --sysname dillon

python3 tools/run_saffire.py launch-bootloader --emulated  --sysname dillon --sock-root socks/ --uart-sock 1234

echo " ---------------"
echo "|RUNNING 8K TEST|
echo " ---------------

python3 tools/run_saffire.py fw-protect --emulated --sysname dillon --fw-root firmware/ --raw-fw-file81k.bin --protected-fw-file example_fw.prot --fw-version 4 --fw-message 'version 4'


python3 tools/run_saffire.py kill-system --emulated --sysname dillon

python3 tools/run_saffire.py launch-bootloader --emulated  --sysname dillon --sock-root socks/ --uart-sock 1234

echo " ---------------"
echo "|RUNNING 9K TEST|
echo " ---------------

python3 tools/run_saffire.py fw-protect --emulated --sysname dillon --fw-root firmware/ --raw-fw-file 9k.bin --protected-fw-file example_fw.prot --fw-version 4 --fw-message 'version 4'

echo " --------------"
echo "| PASSED TESTS |"
echo " --------------"